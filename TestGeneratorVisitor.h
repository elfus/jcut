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
    llvm::Function *mCurrentFunction;
    llvm::BasicBlock *mCurrentBB;
    std::vector<llvm::Instruction*> mInstructions;
    std::vector<llvm::Value*> mArgs;
    std::vector<llvm::Function*> mTests;
    unsigned mCurrentTest;
    unsigned mTestCount;
    
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
    void VisitTestDefinitionExpr(TestDefinitionExpr *);

    
    llvm::Function* nextTest() {
        if(mCurrentTest == mTests.size())
            return nullptr;
        return mTests[mCurrentTest++];
    }
};

#endif	/* TESTGENERATORVISITOR_H */

