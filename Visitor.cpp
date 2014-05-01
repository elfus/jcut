/**
 * @file Visitor.cpp
 * @author Adrian Ortega
 * 
 * Created on May 1, 2014, 9:19 AM
 */

#include "Visitor.hxx"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "TestParser.hxx"

using namespace llvm;

bool TestGeneratorVisitor::VisitTestDefinitionExpr(TestDefinitionExpr *TD)
{
	string test_name = "test_" + TD->getFunctionCall()->getIdentifier()->getIdentifierStr();
	mCurrentFunction = cast<Function> (mModule->getOrInsertFunction(test_name,
			Type::getInt32Ty(mModule->getContext()), // Return type
			(Type*) 0)); // Arguments

	mCurrentBB = BasicBlock::Create(mModule->getContext(), "wrapperBlock", mCurrentFunction);

	mCurrentBuilder = new IRBuilder<>(mCurrentBB);
	return true;
}
