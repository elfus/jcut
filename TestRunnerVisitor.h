/*
 * File:   TestRunnerVisitor.h
 * Author: aortegag
 *
 * Created on May 25, 2014, 3:51 PM
 */

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
    void runFunction(LLVMFunctionHolder* FW) {
        llvm::Function* f = FW->getLLVMFunction();
        if (f) {
            if (mDumpFunctions) {
                f->dump();
            }

            llvm::GenericValue rval = mEE->runFunction(f,mArgs);
            mEE->clearAllGlobalMappings();
            mEE->freeMachineCodeForFunction(f);
            FW->setReturnValue(rval);
        } else
            cerr << "No available LLVM test function! Received nullptr" << endl;
    }
public:
    TestRunnerVisitor() = delete;
    TestRunnerVisitor(const TestRunnerVisitor& orig) = delete;
    TestRunnerVisitor(llvm::ExecutionEngine *EE, bool dump_func = false, llvm::Module* mM=nullptr) : mEE(EE),
        mDumpFunctions(dump_func), mModule(mM) {}
    virtual ~TestRunnerVisitor() { delete mEE; }

    bool isValidExecutionEngine() const { return mEE != nullptr; }

    // @todo Try visiting GlobalSetup
    virtual void VisitTestGroupFirst(TestGroup *TG) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        GlobalSetup* GS = TG->getGlobalSetup();
        if (GS)
            runFunction(GS);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
    }
    // @todo Try visiting GlobalTeardown
    void VisitTestGroup(TestGroup *TG) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        GlobalTeardown* GT = TG->getGlobalTeardown();
        if (GT)
            runFunction(GT);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
    }

    void VisitTestSetup(TestSetup* TS) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TS);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TS->setOutput(std::move(StdCapture::GetCapture()));
    }

    void VisitTestFunction(TestFunction *TF) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TF);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TF->setOutput(std::move(StdCapture::GetCapture()));
    }

    void VisitTestTeardown(TestTeardown *TT) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TT);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TT->setOutput(std::move(StdCapture::GetCapture()));
    }

    void VisitTestDefinition(TestDefinition *TD) {
        runFunction(TD); // do the cleanup
    }

};

#endif	/* TESTRUNNERVISITOR_H */

