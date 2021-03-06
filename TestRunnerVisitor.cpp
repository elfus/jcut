//===-- jcut/TestRunnerVisitor.h --------------------------------*- C++ -*-===//
//
// This file is part of JCUT, A Just-n-time C Unit Testing framework.
//
// Copyright (c) 2014 Adrián Ortega García <adrianog(dot)sw(at)gmail(dot)com>
// All rights reserved.
//
// JCUT is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// JCUT is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with JCUT (See LICENSE.TXT for details).
// If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//
#include "TestRunnerVisitor.h"

// Headers needed to fork!
#ifdef __MINGW32__

#else
#include <unistd.h>
#include <sys/wait.h>
#endif
///////

#include "llvm/Support/CommandLine.h"
extern llvm::cl::opt<bool> NoForkOpt;

void TestRunnerVisitor::runFunction(LLVMFunctionHolder* FW) {
	llvm::Function* f = FW->getLLVMFunction();
	if (f) {
		if(!StdCapture::BeginCapture())
			cerr << "** There was a problem capturing test output!" << endl;
		if (mDumpFunctions) {
			f->dump();
		}

		llvm::GenericValue rval = mEE->runFunction(f,mArgs);
		FW->setReturnValue(rval);
		if(!StdCapture::EndCapture())
			cerr << "** There was a problem finishing the test output capture!" << endl;
		FW->setOutput(StdCapture::GetCapture());
	}
}

/// Executes the MockupFunctions stored in our stack, they are not discarded.
void TestRunnerVisitor::executeMockupFunctionsOnTopOfStack() {
	std::stack<llvm::Function*> backup;
	// Execute the mockups from parent group, but keep the Functions
	while(!mMockupRevert.empty() && mMockupRevert.top() != nullptr) {
		llvm::Function* previous_group_mockup = mMockupRevert.top();
		assert(previous_group_mockup && "Invalid group mockup function");
		mEE->runFunction(previous_group_mockup,mArgs);
		backup.push(previous_group_mockup);
		mMockupRevert.pop();
	}
	// Restore the functions back in the stack
	while(!mMockupRevert.empty() && backup.size()) {
		mMockupRevert.push(backup.top());
		backup.pop();
	}
}

void TestRunnerVisitor::VisitGroupMockup(GlobalMockup *GM) {
	vector<MockupFunction*> mockups =
			GM->getMockupFixture()->getMockupFunctions();
	mMockupRevert.push(nullptr);
	for(MockupFunction* m : mockups) {
		llvm::Function* change_to_mockup = m->getMockupFunction();
		assert(change_to_mockup && "Invalid group mockup function");
			mEE->runFunction(change_to_mockup,mArgs);
			mMockupRevert.push(change_to_mockup);
	}
}

void TestRunnerVisitor::VisitTestGroup(TestGroup *TG) {
	runFunction(TG);
	const GlobalMockup *GM = TG->getGlobalMockup();
	if(GM) {
		while(mMockupRevert.top() != nullptr)
			mMockupRevert.pop(); // Ignore current group
		mMockupRevert.pop(); // Ignore nullptr

		executeMockupFunctionsOnTopOfStack();

		// If the stack is empty we are back at the root group, use the
		// original function
		if(mMockupRevert.empty()) {
			vector<MockupFunction*> mockups =
						GM->getMockupFixture()->getMockupFunctions();
			for(MockupFunction* m : mockups) {
				llvm::Function* change_to_original = m->getOriginalFunction();
				assert(change_to_original && "Invalid group mockup function");
				mEE->runFunction(change_to_original,mArgs);
			}
		}
	}
}



// The test definition
void TestRunnerVisitor::VisitTestDefinition(TestDefinition *TD) {
	TestResults results(mOrder);
	results.mColumnNames = mColumnNames;
	results.using_fork = !NoForkOpt.getValue();
	string test_name = TestResults::getColumnString(TEST_NAME, TD);
	results.mTmpFileName = test_name + "-tmp.txt";
	pid_t pid;

	if(NoForkOpt.getValue() == false) {
#ifdef __MINGW32__
		cout << "Warning: Running test in same address space as jcut" << endl;
		pid = 0;
#else
		if(pipe(results.mPipe) == -1)
			throw JCUTException("Could not create pipes for communication with the test "+test_name);

		pid = fork();
#endif
		if(pid == -1)
			throw JCUTException("Could not fork process for test "+test_name);
	}
	else
		pid = 0;

	if(pid == 0) { // Child process will execute the test
		if(TD->hasTestMockup()) {
			vector<MockupFunction*> mockups=
				TD->getTestMockup()->getMockupFixture()->getMockupFunctions();

			for(MockupFunction* m : mockups) {
				llvm::Function* change_to_mockup = m->getMockupFunction();
				mEE->runFunction(change_to_mockup,mArgs);
			}
		}

		runFunction(TD);

		llvm::Function* func = TD->getLLVMResultFunction();
		if(!func)
			assert(false && "Function test result not found!");
		llvm::GenericValue ret = mEE->runFunction(func, mArgs);
		TD->setPassingValue(ret.IntVal.getBoolValue());

		std::vector<ExpectedExpression*> failing;
		// @bug @todo Debug ExpectedExpressions: Try all possible combinations.
		// When the test does not have an expected result and the expected expression
		// should fail, it always passess.
		for(ExpectedExpression* ptr : mExpExpr) {
			llvm::Function* ee_func = ptr->getLLVMResultFunction();
			if(!ee_func)
				assert(false && "Function expected result result not found!");
			llvm::GenericValue ee_ret = mEE->runFunction(ee_func, mArgs);
			bool passed = ee_ret.IntVal.getBoolValue();
			if(passed == false)
				failing.push_back(ptr);
			TD->setPassingValue(passed);
		}
		mExpExpr.clear();

		// Do the opposite steps for the Mockups
		if(TD->hasTestMockup()) {
			vector<MockupFunction*> mockups =
				TD->getTestMockup()->getMockupFixture()->getMockupFunctions();
			for(MockupFunction* m : mockups) {
				llvm::Function* change_to_original = m->getOriginalFunction();
				mEE->runFunction(change_to_original,mArgs);
			}

			////////////////////////////////////////////////
			// Point to the mockup functions for the current group
			executeMockupFunctionsOnTopOfStack();
		}

		// Include failing ExpectedExpressions from before and after statements.
		if(!failing.empty())
			TD->setFailedExpectedExpressions(failing);

		results.collectTestResults(TD);
		results.saveToDisk();

		if(NoForkOpt.getValue() == false)
#ifdef __MINGW32__
			do {} while(0); // @todo implement fork in windows
#else
			_Exit(EXIT_SUCCESS);
#endif
	} // end of child process
	else { // Continue parent process
#ifdef __MINGW32__
		do {} while(0); // @todo implement fork in windows
#else
		int status = 0;
		waitpid(pid, &status, 0);
		if(WIFEXITED(status)) {
			// Child process ended normally
		} else {
			stringstream ss;
			string function_called = TD->getTestFunction()->
					getFunctionCall()->getFunctionCalledString();
			ss << "The function " << function_called
					<< " crashed during execution. More details:" << endl;
			ss << "I ran that function in a different process but it crashed. Please debug that function" << endl;
			if(WIFSIGNALED(status)) {
				ss << "Child process was terminated by signal: " << strsignal(status) << endl;
				if(WCOREDUMP(status))
					ss << "Child process produced a core dump!" << endl;
			}
			throw JCUTException(ss.str());
		}
#endif
	}

	results.mResults = results.readFromDisk();
	TD->setTestResults(results.mResults);
}




