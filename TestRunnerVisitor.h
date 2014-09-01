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
    std::vector<ExpectedExpression*> mFailedEE;
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

    void VisitTestSetup(TestSetup* TS) {
        runFunction(TS);
    }

    // ExpectedExpressions can be located either in any of after/before or
    // after_all/before_all statements
    void VisitExpectedExpression(ExpectedExpression* EE) {
    	runFunction(EE);
    	// The ExpectedExpression failed.
    	bool passed = EE->getReturnValue().IntVal.getBoolValue();
    	if(false == passed) {
    		mFailedEE.push_back(EE);
    	}
    }

    // The actual function under test
    void VisitTestFunction(TestFunction *TF) {
        runFunction(TF);

        // We used the getPointerToGlobal instead because it works with JIT engine
        // and the previous method works only with MCJIT.
        llvm::GlobalVariable* g = TF->getResultVariable();
        if(g) {
            string result_name = TF->getResultVariable()->getName().str();
            unsigned char* pass = static_cast<unsigned char*>(mEE->getPointerToGlobal(g));
            if(pass) {
                TF->setPassingValue(static_cast<bool>(*pass));
            }
        } else
            assert(false && "Invalid global variable for result!");

    }

    void VisitTestTeardown(TestTeardown *TT) {
        runFunction(TT);
    }

    // The cleanup
    void VisitTestDefinition(TestDefinition *TD) {
        runFunction(TD); // do the cleanup

        // Include failing ExpectedExpressions from before and after statements.
        if(mFailedEE.size()) {
        	TD->getTestFunction()->setFailedExpectedExpressions(mFailedEE);
        	mFailedEE.clear();
		}
    }

};

#endif	/* TESTRUNNERVISITOR_H */

