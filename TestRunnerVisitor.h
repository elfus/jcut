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

using namespace tp;

class TestRunnerVisitor : public Visitor {
private:
    llvm::ExecutionEngine* mEE;
    std::vector<llvm::GenericValue> mArgs;//Dummy arguments
    
    void runFunction(LLVMFunctionWrapper* FW) {
        llvm::Function* f = FW->getLLVMFunction();
        llvm::GenericValue rval = mEE->runFunction(f,mArgs);
        FW->setReturnValue(rval);
    }
public:
    TestRunnerVisitor() = delete;
    TestRunnerVisitor(const TestRunnerVisitor& orig) = delete;
    TestRunnerVisitor(llvm::ExecutionEngine *EE) : mEE(EE) {}
    virtual ~TestRunnerVisitor() { delete mEE; }
    
    bool isValidExecutionEngine() const { return mEE != nullptr; }
    
    virtual void VisitTestGroupFirst(TestGroup *TG) {
        GlobalSetup* GS = TG->getGlobalSetup();
        if (GS)
            runFunction(GS);
    }
    
    void VisitTestGroup(TestGroup *TG) {
        GlobalTeardown* GT = TG->getGlobalTeardown();
        if (GT)
            runFunction(GT);
    }
    
    void VisitTestDefinition(TestDefinition *TD) {
        runFunction(TD);
    }

};

#endif	/* TESTRUNNERVISITOR_H */

