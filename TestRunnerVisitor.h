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
        if (mDumpFunctions) {
            f->dump();
        }

        llvm::GenericValue rval = mEE->runFunction(f,mArgs);
        mEE->clearAllGlobalMappings();
        mEE->freeMachineCodeForFunction(f);
        FW->setReturnValue(rval);
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
        GlobalSetup* GS = TG->getGlobalSetup();
        if (GS)
            runFunction(GS);
    }
    // @todo Try visiting GlobalTeardown
    void VisitTestGroup(TestGroup *TG) {
        GlobalTeardown* GT = TG->getGlobalTeardown();
        if (GT)
            runFunction(GT);
    }

    void VisitTestDefinition(TestDefinition *TD) {
        TestMockup* TM = TD->getTestMockup();
        if (TM) {
            TM->useMockupFunction();
             string target_func = TD->getTestFunction()->getFunctionCall()->getIdentifier()->getIdentifierStr();
            llvm::Function* f = mModule->getFunction(target_func);
            assert(f && "Could not get target function");
            f->dump();
            mEE->recompileAndRelinkFunction(f);
        }
        if(!StdCapture::BeginCapture())
            cerr << "** There was a problem capturing test output!" << endl;
        runFunction(TD);
        if(!StdCapture::EndCapture())
            cerr << "** There was a problem finishing the test output capture!" << endl;
        TD->setTestOutput(std::move(StdCapture::GetCapture()));
        if (TM) {
            TM->useOriginalFunction(mEE);
        }
    }

};

#endif	/* TESTRUNNERVISITOR_H */

