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
#include "TestParser.h"

using namespace llvm;

TestGeneratorVisitor::TestGeneratorVisitor(llvm::Module *mod) :
mModule(mod),
mBuilder(mod->getContext()),
mTestFunctionCall(nullptr),
mCurrentBB(nullptr),
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
	FunctionCall* parent = arg->getParent();
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

void TestGeneratorVisitor::VisitFunctionCall(FunctionCall *FC)
{
	string func_name = FC->getIdentifier()->getIdentifierStr();
	Function *funcToBeCalled = mModule->getFunction(func_name);
	assert(funcToBeCalled != nullptr && "Function not found!");
	CallInst *call = mBuilder.CreateCall(funcToBeCalled, mArgs);

        if (funcToBeCalled->getReturnType()->getTypeID() == Type::TypeID::VoidTyID)
               mReturnValue = mBuilder.getInt32(1);// 1 Passes, 0 fails for void functions
        else
               mReturnValue = call;

        mTestFunctionCall = call;
	mInstructions.push_back(call);
	mArgs.clear();
}

void TestGeneratorVisitor::VisitExpectedResult(ExpectedResult *ER)
{
	// Get the call instruction pushed by VisitFunctionCallExpr
    CallInst *call = dyn_cast<CallInst>(mInstructions.back());
    if(call == nullptr)
        throw Exception("Invalid CallInst!");

	if(call->getCalledFunction()->getReturnType()->getTypeID() == Type::TypeID::VoidTyID) {
		cerr<<"Warning: Trying compare a value against a function with no return value"<<endl;
		return;
	}

	stringstream ss;
	int tmp = ER->getExpectedConstant()->getValue();//workaround, convert int to string
	ss << tmp;
	llvm::Value* c = createValue(call->getType(), ss.str());

	string InstName = "ComparisonInstruction";
    Value* i = nullptr;
    switch(ER->getComparisonOperator()->getType()) {
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

void TestGeneratorVisitor::VisitExpectedExpression(ExpectedExpression *EE)
{
	Operand* LHS = EE->getLHSOperand();
	Operand* RHS = EE->getRHSOperand();
	ComparisonOperator* CO = EE->getComparisonOperator();
	Value* L = nullptr;
	Value* R = nullptr;

	if (LHS->isIdentifier()) {
		llvm::GlobalVariable* g = mModule->getGlobalVariable(LHS->getIdentifier()->getIdentifierStr());
		assert(g && "LHS Operator not found!");
		L = mBuilder.CreateLoad(g);
		mInstructions.push_back((llvm::Instruction*)L);
	}

	if (RHS->isIdentifier()) {
		llvm::GlobalVariable* g = mModule->getGlobalVariable(RHS->getIdentifier()->getIdentifierStr());
		assert(g && "RHS Operator not found!");
		R = mBuilder.CreateLoad(g);
		mInstructions.push_back((llvm::Instruction*)R);
	}

	if (LHS->isConstant()) {
		if (R != nullptr) {
			stringstream ss;
			ss <<  LHS->getConstant()->getValue();
			L = createValue(R->getType(),ss.str());
		}
	}

	if (RHS->isConstant()) {
		if (L != nullptr) {
			stringstream ss;
			ss <<  RHS->getConstant()->getValue();
			R = createValue(L->getType(),ss.str());
		}
	}

	string InstName = "ExpExprComp";
	Value* i = nullptr;
    switch(CO->getType()) {
        case ComparisonOperator::EQUAL_TO:
			i = mBuilder.CreateICmpEQ(L, R, InstName);
            break;
        case ComparisonOperator::NOT_EQUAL_TO:
			i = mBuilder.CreateICmpNE(L, R, InstName);
            break;
        case ComparisonOperator::GREATER_OR_EQUAL:
            i = mBuilder.CreateICmpSGE(L, R, InstName);
            break;
        case ComparisonOperator::LESS_OR_EQUAL:
            i = mBuilder.CreateICmpSLE(L, R, InstName);
            break;
        case ComparisonOperator::GREATER:
            i = mBuilder.CreateICmpSGT(L, R, InstName);
            break;
        case ComparisonOperator::LESS:
            i = mBuilder.CreateICmpSLT(L, R, InstName);
            break;
        case ComparisonOperator::INVALID:
            break;
    }
	assert(i && "Invalid ComparisonOperator");

	llvm::ZExtInst* zext = (llvm::ZExtInst*) mBuilder.CreateZExt(i,mReturnValue->getType());
	llvm::Value* mainBool = mBuilder.CreateICmpEQ(mReturnValue, zext);
	llvm::ZExtInst* zext2 = (llvm::ZExtInst*) mBuilder.CreateZExt(mainBool,mReturnValue->getType());

	mInstructions.push_back((llvm::Instruction*)i);
	mInstructions.push_back(zext);
    mInstructions.push_back((llvm::Instruction*)mainBool);
	mInstructions.push_back(zext2);
	mReturnValue = zext2;
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
void TestGeneratorVisitor::VisitVariableAssignment(VariableAssignment *VA)
{
	string variable_name = VA->getIdentifier()->getIdentifierStr();
	GlobalVariable* global_variable = mModule->getGlobalVariable(variable_name);
	assert(global_variable && "Variable not found!");
	Tokenizer::Token tokenType = VA->getArgument()->getTokenType();
	string real_value = VA->getArgument()->getStringRepresentation();

	/// @todo Add support for structures
	LoadInst* load_value = mBuilder.CreateLoad(global_variable);
	mBackup.push_back(make_tuple(load_value, global_variable));
	mInstructions.push_back(load_value);

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

void TestGeneratorVisitor::VisitTestDefinition(TestDefinition *TD)
{
	string func_name = TD->getTestFunction()->getFunctionCall()->getIdentifier()->getIdentifierStr();
	string test_name = "test_" + func_name + "_0";
	unsigned i = 0;

	do {
		test_name.pop_back();
		test_name = test_name + ((char) (((int) '0') + i));
		++i;
	} while (mModule->getFunction(test_name));

	TD->setTestName(test_name);

	Function *testFunction = generateFunction(test_name,true,true);

	TD->setLLVMFunction(testFunction);
}

void TestGeneratorVisitor::VisitGlobalSetup(GlobalSetup *GS)
{
	string func_name = "setup_"+GS->getGroupName();
	saveGlobalVariables();
	Function *testFunction = generateFunction(func_name);
	GS->setLLVMFunction(testFunction);
}

void TestGeneratorVisitor::VisitGlobalTeardown(GlobalTeardown *GT)
{
	string func_name = "teardown_"+GT->getGroupName();
	restoreGlobalVariables();
	Function *testFunction = generateFunction(func_name);
	GT->setLLVMFunction(testFunction);
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

llvm::Function* TestGeneratorVisitor::generateFunction(const string& name,
													bool use_mReturnValue,
													bool restore_backup)
{
	Function *function = cast<Function> (mModule->getOrInsertFunction(
			name,
			Type::getInt32Ty(mModule->getContext()),
			(Type*) 0));
	BasicBlock *BB = BasicBlock::Create(mModule->getContext(),
			"block_" + name, function);

	if(restore_backup) {
		for(tuple<llvm::Value*,llvm::GlobalVariable*>& tup : mBackup) {
			StoreInst* st = mBuilder.CreateStore(get<0>(tup), get<1>(tup));
			mInstructions.push_back(st);
		}
	}

	ReturnInst *ret = nullptr;
	if(use_mReturnValue)
		ret = mBuilder.CreateRet(mReturnValue);
	else
		ret = mBuilder.CreateRet(mBuilder.getInt32(0));

	mInstructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : mInstructions)
		mBuilder.Insert(inst);

	mInstructions.clear();
	mBackup.clear();
	mBuilder.ClearInsertionPoint();

	return function;
}

void TestGeneratorVisitor::saveGlobalVariables()
{
	llvm::GlobalVariable* dst = nullptr;
	mBackupGroup.push_back(make_tuple(nullptr,nullptr));
	for(tuple<llvm::Value*,llvm::GlobalVariable*>& tup : mBackup) {
		Value* g_value = get<0>(tup);
		GlobalVariable* original = get<1>(tup);
		/// @todo Watch for pointer types and structures
		dst = new GlobalVariable(*mModule,
				original->getType()->getElementType(),
				false,
				GlobalValue::LinkageTypes::ExternalLinkage,
				0,
				"backup_"+original->getName().str(),
				original);
		dst->setInitializer(original->getInitializer());
		dst->copyAttributesFrom(original);
		StoreInst* st = mBuilder.CreateStore(g_value, dst);
		mInstructions.push_back(st);

		mBackupGroup.push_back(make_tuple(dst,original));
	}
}

void TestGeneratorVisitor::restoreGlobalVariables()
{
	// Restore the backed up variables from the global variables created in
	// VisitGlobalSetup
	// @bug This works only where we declare a teardown with "after_all"
	while(mBackupGroup.size()) {
		tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>& tup = mBackupGroup.back();
		mBackupGroup.pop_back();
		if(get<0>(tup)==nullptr and get<1>(tup)==nullptr)
			break;
		LoadInst* ld = mBuilder.CreateLoad(get<0>(tup));
		StoreInst* st = mBuilder.CreateStore(ld, get<1>(tup));
		mInstructions.push_back(ld);
		mInstructions.push_back(st);
	}
}


