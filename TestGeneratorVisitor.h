/* 
 * File:   TestGeneratorVisitor.h
 * Author: aortegag
 *
 * Created on May 1, 2014, 7:39 PM
 */

#ifndef TESTGENERATORVISITOR_H
#define	TESTGENERATORVISITOR_H

#include "Visitor.hxx"
#include <string>
#include <vector>

namespace llvm {
    class Function;
    class Module;
    class BasicBlock;
}

#include "llvm/IR/IRBuilder.h"
#include "TestParser.hxx"

using namespace tp;

class TestGeneratorVisitor : public Visitor {
private:
    llvm::Module *mModule;
    llvm::IRBuilder<> mBuilder;
    /// Used as a temp variable to the instruction that calls the function to be tested.
    llvm::CallInst *mTestFunctionCall;
    /// Every time we visit a TestDefinitionExpr a new BasicBlock is created.
    llvm::BasicBlock *mCurrentBB;
    /// Instructions generated for each TestDefinitionExpr
    std::vector<llvm::Instruction*> mInstructions;
    /// Used to hold the arguments for each FunctionCallExpr
    std::vector<llvm::Value*> mArgs;
    // llvm::Function's created by this Visitor
    std::vector<llvm::Function*> mTests;
    llvm::Function *mGlobalSetup;
    llvm::Function *mGlobalTeardown;
    unsigned mCurrentTest;
    unsigned mTestCount;
    llvm::Value *mReturnValue;
    
    /**
 * Creates a new Value of the same Type as type with real_value
 */
    llvm::Value* createValue(llvm::Type* type,
                             const std::string& real_value);
public:
    TestGeneratorVisitor(llvm::Module *mod);
    TestGeneratorVisitor(const TestGeneratorVisitor&) = delete;
    ~TestGeneratorVisitor() {}
    
    void VisitFunctionArgument(FunctionArgument *);
    void VisitFunctionCallExpr(FunctionCallExpr *);
    void VisitExpectedResult(ExpectedResult *);
    void VisitTestFunction(TestFunction *);
    void VisitVariableAssignmentExpr(VariableAssignmentExpr *);
    void VisitTestDefinitionExpr(TestDefinitionExpr *);
    void VisitGlobalSetupExpr(GlobalSetupExpr *);
    void VisitGlobalTeardownExpr(GlobalTeardownExpr *);
    

    
    llvm::Function* nextTest() {
        if(mCurrentTest == mTests.size())
            return nullptr;
        return mTests[mCurrentTest++];
    }
    
    
    llvm::Function* getGlobalSetup() const { return mGlobalSetup; }
    llvm::Function* getGlobalTeardown() const { return mGlobalTeardown; }
};

#endif	/* TESTGENERATORVISITOR_H */

