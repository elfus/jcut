#include "TestRunnerVisitor.h"

// Headers needed to fork!
#include <unistd.h>
#include <sys/wait.h>
///////

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
	string test_name = TestResults::getColumnString(TEST_NAME, TD);
	results.mTmpFileName = test_name + "-tmp.txt";
	pid_t pid;

	pid = fork();
	if(pid == -1)
		throw JCUTException("Could not fork process for test"+test_name);

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
			llvm::Function* ee_func = TD->getLLVMResultFunction();
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
		if(failing.size()) {
			TD->setFailedExpectedExpressions(failing);
		}
		results.collectTestResults(TD);
		results.saveToDisk();
		_Exit(EXIT_SUCCESS);
	} // end of child process
	else { // Continue parent process
		int status = 0;
		waitpid(pid, &status, 0);
		if(WIFEXITED(status)) {
			// Child process ended normally
		} else {
			if(WIFSIGNALED(status)) {
				cerr << "Child was terminated by signal: " << WTERMSIG(status) << endl;
				if(WCOREDUMP(status))
					cerr << "Child produced a core dump!" << endl;
			}
			cout << "Child ended with status "<< status << endl;
		}
	}

	results.mResults = results.readFromDisk();
	TD->setTestResults(results.mResults);
}




