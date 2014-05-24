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
mTestFunctionCall(nullptr),
mCurrentBB(nullptr),
mGlobalSetup(nullptr),
mGlobalTeardown(nullptr),
mCurrentTest(0),
mTestCount(0),
mReturnValue(nullptr)
{
}

/**
 * Create the appropriate instructions for each argument type
 *
 * @TODO Improve implementation and code readability
 * @TODO Add support for pointers to pointers and more complex types
 *
 * @param arg
 * @return
 */
void TestGeneratorVisitor::VisitFunctionArgument(tp::FunctionArgument *arg)
{
	FunctionCallExpr* parent = arg->getParent();
	string func_name = parent->getIdentifier()->getIdentifierStr();
	llvm::Function *currentFunction = mModule->getFunction(func_name);
	llvm::Function::arg_iterator arg_it = currentFunction->arg_begin();

	unsigned i = 0;
	while (arg_it != currentFunction->arg_end()) {
		if (i == arg->getIndex()) {
			llvm::Argument& llvm_arg = *arg_it;

			if (llvm_arg.getType()->getTypeID() == Type::TypeID::PointerTyID) {
				BufferAlloc *ba = arg->getBufferAlloc();
				assert(ba != nullptr && "Invalid BufferAlloc pointer");
				AllocaInst *alloc1 = mBuilder.CreateAlloca(
						llvm_arg.getType()->getPointerElementType(),
						mBuilder.getInt(APInt(32, ba->getBufferSize(), 10))
						); // Allocate memory for the element type pointed to

				Value *v = createValue(llvm_arg.getType()->getPointerElementType(), ba->getDefaultValue());
				StoreInst *store1 = mBuilder.CreateStore(v, alloc1);

				// Allocate memory for a pointer type
				AllocaInst *alloc2 = mBuilder.CreateAlloca(llvm_arg.getType(), 0, "AllocPtr" + Twine(i)); // Allocate a pointer type
				alloc2->setAlignment(8);
				StoreInst *store2 = mBuilder.CreateStore(alloc1, alloc2); // Store an already allocated variable address to our pointer
				LoadInst *load = mBuilder.CreateLoad(alloc2, "value3"); // Load whatever address is in alloc3 into load3 'value3'

				mInstructions.push_back(alloc1);
				mInstructions.push_back(store1);
				mInstructions.push_back(alloc2);
				mInstructions.push_back(store2);
				mInstructions.push_back(load);
				mArgs.push_back(load);
				break;
			}
			// Code for non-pointer types
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
}

void TestGeneratorVisitor::VisitFunctionCallExpr(FunctionCallExpr *FC)
{
	string func_name = FC->getIdentifier()->getIdentifierStr();
	Function *funcToBeCalled = mModule->getFunction(func_name);
	assert(funcToBeCalled != nullptr && "Function not found!");
	CallInst *call = mBuilder.CreateCall(funcToBeCalled, mArgs);

        if (funcToBeCalled->getReturnType()->getTypeID() == Type::TypeID::VoidTyID)
               mReturnValue = mBuilder.getInt32(0);
        else
               mReturnValue = call;

        mTestFunctionCall = call;
	mInstructions.push_back(call);
	mArgs.clear();
}

// ComparisonOperator will be visited after FunctionCallExpr
void TestGeneratorVisitor::VisitComparisonOperator(ComparisonOperator *CO)
{
    // Get the call instruction pushed by VisitFunctionCallExpr
    CallInst *call = dyn_cast<CallInst>(mInstructions.back());
    if(call == nullptr)
        throw Exception("Invalid CallInst!");

    // @todo: Modify the constant against which we are comparing
    // this will have to be done when visitting the expected-constant
	llvm::Constant* c = mBuilder.getInt32(5); // Give it a name so we can modify it later

	string InstName = "ComparisonInstruction";
    Value* i = nullptr;
    switch(CO->getType()) {
        case ComparisonOperator::EQUAL_TO:
			i = mBuilder.CreateICmpEQ(call, c, InstName);
            break;
        case ComparisonOperator::NOT_EQUAL_TO:
			i = mBuilder.CreateICmpNE(call, c, InstName);
            break;
        case ComparisonOperator::GREATER_OR_EQUAL:
            i = mBuilder.CreateICmpSGE(call, c, InstName);
            break;
        case ComparisonOperator::LESS_OR_EQUAL:
            i = mBuilder.CreateICmpSLE(call, c, InstName);
            break;
        case ComparisonOperator::GREATER:
            i = mBuilder.CreateICmpSGT(call, c, InstName);
            break;
        case ComparisonOperator::LESS:
            i = mBuilder.CreateICmpSLT(call, c, InstName);
            break;
        case ComparisonOperator::INVALID:
            break;
    }

    mInstructions.push_back((llvm::Instruction*)i);
    // Convert the i1 integer to a i32 integer
    llvm::ZExtInst* zext = (llvm::ZExtInst*) mBuilder.CreateZExt(i,c->getType());
    mInstructions.push_back(zext);
    mReturnValue = zext;
}

void TestGeneratorVisitor::VisitExpectedConstant(ExpectedConstant *EC)
{
	vector<llvm::Instruction*>::reverse_iterator it;
	llvm::Instruction* inst = nullptr;
	for (it = mInstructions.rbegin(); it != mInstructions.rend(); ++it) {
		inst = *it;
		if (inst->getName() == "ComparisonInstruction") {
			// This operand is the constant created in the VisitComparisonOperator method
			llvm::Constant* c = mBuilder.getInt32(EC->getValue());
			inst->setOperand(1,c);
			break;
		}
	}
}

void TestGeneratorVisitor::VisitTestFunction(TestFunction *TF)
{

}

/**
 * Creates LLVM IR code for a single global variable assignment.
 *
 * @todo Support the rest of assignment types
 * @param VA
 */
void TestGeneratorVisitor::VisitVariableAssignmentExpr(VariableAssignmentExpr *VA)
{
	string variable_name = VA->getIdentifier()->getIdentifierStr();
	GlobalVariable* global_variable = mModule->getGlobalVariable(variable_name);
	assert(global_variable && "Variable not found!");
	Tokenizer::Token tokenType = VA->getArgument()->getTokenType();
	string real_value = VA->getArgument()->getStringRepresentation();

	// TODO: Handle the rest of token types
	switch (tokenType) {
	case Tokenizer::TOK_INT:
	{
		// Watch for the real type of a global variable, there might be a @bug
		Value * v = createValue(global_variable->getType()->getPointerElementType(), real_value);
		StoreInst *store = mBuilder.CreateStore(v, global_variable);
		mInstructions.push_back(store);
	}
		break;
	default:
		assert(false && "Unhandled case! FIX ME");
		break;
	}
}

void TestGeneratorVisitor::VisitTestDefinitionExpr(TestDefinitionExpr *TD)
{
	string func_name = TD->getTestFunction()->getFunctionCall()->getIdentifier()->getIdentifierStr();
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

        ReturnInst *ret = mBuilder.CreateRet(mReturnValue);
        mInstructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : mInstructions)
		mBuilder.Insert(inst);

	mInstructions.clear();
	mBuilder.ClearInsertionPoint();

	mTests.push_back(testFunction);
}

void TestGeneratorVisitor::VisitGlobalSetupExpr(GlobalSetupExpr *GS)
{
	string func_name = "global_setup";

	Function *testFunction = cast<Function> (mModule->getOrInsertFunction(
			func_name,
			Type::getInt32Ty(mModule->getContext()),
			(Type*) 0));
	BasicBlock *BB = BasicBlock::Create(mModule->getContext(),
			"wrapper_block_" + func_name, testFunction);

	ReturnInst *ret = mBuilder.CreateRet(mBuilder.getInt32(0));

	mInstructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : mInstructions)
		mBuilder.Insert(inst);

	mInstructions.clear();
	mBuilder.ClearInsertionPoint();

	mGlobalSetup = testFunction;
}

void TestGeneratorVisitor::VisitGlobalTeardownExpr(GlobalTeardownExpr *GT)
{
	string func_name = "global_teardown";

	Function *testFunction = cast<Function> (mModule->getOrInsertFunction(
			func_name,
			Type::getInt32Ty(mModule->getContext()),
			(Type*) 0));
	BasicBlock *BB = BasicBlock::Create(mModule->getContext(),
			"wrapper_block_" + func_name, testFunction);

	ReturnInst *ret = mBuilder.CreateRet(mBuilder.getInt32(0));

	mInstructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : mInstructions)
		mBuilder.Insert(inst);

	mInstructions.clear();
	mBuilder.ClearInsertionPoint();

	mGlobalTeardown = testFunction;
}


/**
 * Creates a new Value of the same Type as type with real_value
 *
 * @TODO Improve the way this method works and its documentation
 * @BUG This method only works with IntegerTyID types
 */
llvm::Value* TestGeneratorVisitor::createValue(llvm::Type* type,
		const string& real_value)
{
	Type::TypeID typeID = type->getTypeID();
	switch (typeID) {
	case Type::TypeID::IntegerTyID:
		if (IntegerType * intType = dyn_cast<IntegerType>(type))
			// Watch for the radix, right now we use radix 10 only
			return cast<Value>(mBuilder.getInt(APInt(intType->getBitWidth(), real_value, 10)));
		break;
	case Type::TypeID::FloatTyID:
		break;
	case Type::TypeID::PointerTyID:
		if (PointerType * ptrType = dyn_cast<PointerType>(type)) {
			return ConstantPointerNull::get(ptrType);
		}
	default:
		return nullptr;
	}
	return nullptr;
}


