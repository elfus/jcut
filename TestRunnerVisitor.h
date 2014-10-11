//===-- jcut/TestRunnerVisitor.h - Grammar Rules ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//

#ifndef TESTRUNNERVISITOR_H
#define	TESTRUNNERVISITOR_H

#include "TestParser.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include "OSRedirect.h"

using namespace tp;

// actually define vars.
int StdCapture::m_pipe[2];
int StdCapture::m_oldStdOut;
int StdCapture::m_oldStdErr;
bool StdCapture::m_capturing;
std::mutex StdCapture::m_mutex;
std::string StdCapture::m_captured;

class TestRunnerVisitor : public Visitor {
private:
    llvm::ExecutionEngine* mEE;
    std::vector<llvm::GenericValue> mArgs;//Dummy arguments
    bool mDumpFunctions;
    llvm::Module* mModule;
    // Temp vector to hold failed expected expressions
    std::vector<ExpectedExpression*> mExpExpr;
    /// Used to properly revert the mockup replacements for nested groups.
    /// Every time we enter in a group we store all of its MoclupFunctions
    /// in the stack. This used by individual tests and when returning to a
    /// different group.
    std::stack<llvm::Function*> mMockupRevert;
    void runFunction(LLVMFunctionHolder* FW) {
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
    void executeMockupFunctionsOnTopOfStack() {
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
public:
    TestRunnerVisitor() = delete;
    TestRunnerVisitor(const TestRunnerVisitor& orig) = delete;
    TestRunnerVisitor(llvm::ExecutionEngine *EE, bool dump_func = false, llvm::Module* mM=nullptr) : mEE(EE),
        mDumpFunctions(dump_func), mModule(mM) {}
    virtual ~TestRunnerVisitor() { delete mEE; }

    bool isValidExecutionEngine() const { return mEE != nullptr; }

    void VisitGroupMockup(GlobalMockup *GM) {
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

    void VisitGroupSetup(GlobalSetup *GS) {
        runFunction(GS);
    }

    void VisitGroupTeardown(GlobalTeardown *GT) {
        runFunction(GT);
    }

    // The cleanup
    void VisitTestGroup(TestGroup *TG) {
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

    // ExpectedExpressions can be located either in any of after/before or
    // after_all/before_all statements
    void VisitExpectedExpression(ExpectedExpression* EE) {
    	mExpExpr.push_back(EE);
    }


    // The test definition
    void VisitTestDefinition(TestDefinition *TD) {

    	if(TD->hasTestMockup()) {
    		vector<MockupFunction*> mockups=
    			TD->getTestMockup()->getMockupFixture()->getMockupFunctions();

			for(MockupFunction* m : mockups) {
				llvm::Function* change_to_mockup = m->getMockupFunction();
				mEE->runFunction(change_to_mockup,mArgs);
			}
    	}

        runFunction(TD); // do the cleanup

        llvm::GlobalVariable* g = TD->getGlobalVariable();
		if(g) {
			unsigned char* pass = static_cast<unsigned char*>(mEE->getPointerToGlobal(g));
			if(pass) {
				TD->setPassingValue(static_cast<bool>(*pass));
			} else
				assert(false && "Test result Global variable not found!");
		} else
			assert(false && "Invalid global variable for result!");

		std::vector<ExpectedExpression*> failing;
		for(ExpectedExpression* ptr : mExpExpr) {
			llvm::GlobalVariable* v = ptr->getGlobalVariable();
			if(v) {
				unsigned char* pass = static_cast<unsigned char*>(mEE->getPointerToGlobal(v));
				if(pass) {
					bool passed = static_cast<bool>(*pass);
					TD->setPassingValue(passed);
					if(!passed)
						failing.push_back(ptr);
				} else
					assert(false && "ExpectedExpression Global variable not found!");
			} else {
				assert(false &&  "Invalid global variable for expected expression");
			}
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
    }

};

#endif	/* TESTRUNNERVISITOR_H */

