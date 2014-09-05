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
    void runFunction(LLVMFunctionHolder* FW) {
        llvm::Function* f = FW->getLLVMFunction();
        if (f) {
            if(!StdCapture::BeginCapture())
                cerr << "** There was a problem capturing test output!" << endl;
            if (mDumpFunctions) {
                f->dump();
            }

            llvm::GenericValue rval = mEE->runFunction(f,mArgs);
            //////////////////////////////////////
            // @warning Watch out for this method. If you call it you will loose
            // all the current values of your global variables!
            // mEE->clearAllGlobalMappings();
            /////////////////////////////////////
            // @bug this method makes the code crash when VisitTestDefinition.
            // Seems it has to be called in conjunction with the previous one.
            // mEE->freeMachineCodeForFunction(f);
            /////////////////////////////////////
            FW->setReturnValue(rval);
            if(!StdCapture::EndCapture())
                cerr << "** There was a problem finishing the test output capture!" << endl;
            FW->setOutput(StdCapture::GetCapture());
        }
    }
public:
    TestRunnerVisitor() = delete;
    TestRunnerVisitor(const TestRunnerVisitor& orig) = delete;
    TestRunnerVisitor(llvm::ExecutionEngine *EE, bool dump_func = false, llvm::Module* mM=nullptr) : mEE(EE),
        mDumpFunctions(dump_func), mModule(mM) {}
    virtual ~TestRunnerVisitor() { delete mEE; }

    bool isValidExecutionEngine() const { return mEE != nullptr; }

    void VisitGlobalSetup(GlobalSetup *GS) {
        runFunction(GS);
    }

    void VisitGlobalTeardown(GlobalTeardown *GT) {
        runFunction(GT);
    }

    // The cleanup
    void VisitTestGroup(TestGroup *TG) {
        runFunction(TG);
    }

    // ExpectedExpressions can be located either in any of after/before or
    // after_all/before_all statements
    void VisitExpectedExpression(ExpectedExpression* EE) {
    	mExpExpr.push_back(EE);
    }


    // The cleanup
    void VisitTestDefinition(TestDefinition *TD) {
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

        // Include failing ExpectedExpressions from before and after statements.
        if(failing.size()) {
        	TD->setFailedExpectedExpressions(failing);
		}
    }

};

#endif	/* TESTRUNNERVISITOR_H */

