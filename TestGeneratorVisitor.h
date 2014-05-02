/* 
 * File:   TestGeneratorVisitor.h
 * Author: aortegag
 *
 * Created on May 1, 2014, 7:39 PM
 */

#ifndef TESTGENERATORVISITOR_H
#define	TESTGENERATORVISITOR_H

#include "Visitor.hxx"

namespace llvm {
    class Function;
    class Module;
    class BasicBlock;
}

#include "llvm/IR/IRBuilder.h"

class TestGeneratorVisitor : public Visitor {
private:
    llvm::Function *mCurrentFunction;
    llvm::BasicBlock *mCurrentBB;
    llvm::IRBuilder<> *mCurrentBuilder;
    llvm::Module *mModule;
    unsigned mTestCount;
public:
    TestGeneratorVisitor(llvm::Module *mod) : mCurrentFunction(nullptr),
        mModule(mod), mTestCount(0){}
    TestGeneratorVisitor() = delete;
    TestGeneratorVisitor(const TestGeneratorVisitor&) = delete;
    ~TestGeneratorVisitor() {}
    
    bool VisitTestDefinitionExpr(TestDefinitionExpr *);
    bool VisitFunctionCallExpr(FunctionCallExpr*);
    
    llvm::Function* nextFunction() const {
        return mCurrentFunction;
    }
};

#endif	/* TESTGENERATORVISITOR_H */

