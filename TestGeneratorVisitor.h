/* 
 * File:   TestGeneratorVisitor.h
 * Author: aortegag
 *
 * Created on May 1, 2014, 7:39 PM
 */

#ifndef TESTGENERATORVISITOR_H
#define	TESTGENERATORVISITOR_H

#include "Visitor.h"
#include <string>
#include <vector>
#include <tuple>

namespace llvm {
    class Function;
    class Module;
    class BasicBlock;
}

#include "llvm/IR/IRBuilder.h"
#include "TestParser.h"

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
    /// Used to hold backup values of global variables and their original values
    /// @todo Add support for structures
    std::vector<tuple<llvm::Value*,llvm::GlobalVariable*>> mBackup;
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
    void VisitFunctionCall(FunctionCall *);
    void VisitExpectedResult(ExpectedResult *);
    void VisitExpectedExpression(ExpectedExpression *);
    void VisitTestFunction(TestFunction *);
    void VisitVariableAssignment(VariableAssignment *);
    void VisitTestDefinition(TestDefinition *);
    void VisitGlobalSetup(GlobalSetup *);
    void VisitGlobalTeardown(GlobalTeardown *);
    
};

#endif	/* TESTGENERATORVISITOR_H */

