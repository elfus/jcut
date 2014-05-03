/**
 * @file Visitor.cpp
 * @author Adrian Ortega
 * 
 * Created on May 1, 2014, 9:19 AM
 */

#include "TestGeneratorVisitor.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "TestParser.hxx"

using namespace llvm;

TestGeneratorVisitor::TestGeneratorVisitor(llvm::Module *mod) :
mModule(mod),
mBuilder(mod->getContext()),
mCurrentFunction(nullptr),
mCurrentBB(nullptr),
mTestCount(0)
{
}

/**
 * Create the appropriate instructions for each argument type
 *
 * @TODO Add support for pointers
 * 
 * @param arg
 * @return
 */
bool TestGeneratorVisitor::VisitFunctionArgument(tp::FunctionArgument *arg)
{
	const Tokenizer::Token &tokenType = arg->getTokenType();
	FunctionCallExpr* parent = arg->getParent();
	string func_name = parent->getIdentifier()->getIdentifierStr();
	llvm::Function *currentFunction = mModule->getFunction(func_name);
	llvm::Function::arg_iterator arg_it = currentFunction->arg_begin();

	unsigned i = 0;
	while (arg_it != currentFunction->arg_end()) {
		if (i == arg->getIndex()) {
			llvm::Argument& llvm_arg = *arg_it;

			llvm::AllocaInst *alloc = mBuilder.CreateAlloca(llvm_arg.getType(), 0, "Allocation" + Twine(i));
			alloc->setAlignment(4);

			Value *v = createValue(llvm_arg.getType(), arg->getStringRepresentation());
			StoreInst * store = mBuilder.CreateStore(v, alloc);
			LoadInst *load = mBuilder.CreateLoad(alloc, "value" + Twine(i));

			mInstructions.push_back(alloc);
			mInstructions.push_back(store);
			mInstructions.push_back(load);
			mArgs.push_back(load);
			break;
		}
		++i;
		++arg_it;
	}

	if (tokenType == Tokenizer::TOK_BUFF_ALLOC) {
		return true;
	}
	return true;
}

bool TestGeneratorVisitor::VisitFunctionCallExpr(tp::FunctionCallExpr* FC)
{
	// Complete the function calL
	string func_name = FC->getIdentifier()->getIdentifierStr();
	string test_name = "test_" + func_name + "_0";
	unsigned i = 0;

	do {
		test_name.pop_back();
		test_name = test_name + ((char) (((int) '0') + i));
		++i;
	} while (mModule->getFunction(test_name));
	
	Function *testFunction = cast<Function> (mModule->getOrInsertFunction(
			test_name,
			Type::getInt32Ty(mModule->getContext()),
			(Type*) 0));
	BasicBlock *BB = BasicBlock::Create(mModule->getContext(),
			"wrapper_block_" + func_name, testFunction);
	Function *funcToBeCalled = mModule->getFunction(FC->getIdentifier()->getIdentifierStr());
	CallInst *call = mBuilder.CreateCall(funcToBeCalled, mArgs);
	ReturnInst *ret = mBuilder.CreateRet(call);

	mInstructions.push_back(call);
	mInstructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : mInstructions)
		mBuilder.Insert(inst);

	mInstructions.clear();
	mArgs.clear();
	mBuilder.ClearInsertionPoint();

	testFunction->dump();

	mCurrentFunction = testFunction;
	return true;
}

/**
 * Creates a new Value of the same Type as type with real_value
 */
llvm::Value* TestGeneratorVisitor::createValue(llvm::Type* type,
		const string& real_value)
{
	switch (type->getTypeID()) {
	case Type::TypeID::IntegerTyID:
		if (IntegerType * intType = dyn_cast<IntegerType>(type))
			// Watch for the radix, right now we use radix 10 only
			return cast<Value>(mBuilder.getInt(APInt(intType->getBitWidth(), real_value, 10)));
		break;
	case Type::TypeID::FloatTyID:
		break;
	default:
		return nullptr;
	}
	return nullptr;
}


