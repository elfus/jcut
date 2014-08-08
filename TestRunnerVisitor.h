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
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(GS);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        GS->setOutput(StdCapture::GetCapture());
    }

    void VisitGlobalTeardown(GlobalTeardown *GT) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(GT);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        GT->setOutput(StdCapture::GetCapture());
    }

    // The cleanup
    void VisitTestGroup(TestGroup *TG) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TG);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TG->setOutput(StdCapture::GetCapture());
    }

    void VisitTestSetup(TestSetup* TS) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TS);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TS->setOutput(std::move(StdCapture::GetCapture()));
    }

    // The actual function under test
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

    // The cleanup
    void VisitTestDefinition(TestDefinition *TD) {
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TD); // do the cleanup
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TD->setOutput(StdCapture::GetCapture());
    }

};

#endif	/* TESTRUNNERVISITOR_H */

