//===-- jcut/TestGeneratorVisitor.cpp - Code generation ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//

#include "TestGeneratorVisitor.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "TestParser.h"
#include "JCUTScanner.h"

using namespace llvm;

TestGeneratorVisitor::TestGeneratorVisitor(llvm::Module *mod) :
mModule(mod),
mBuilder(mod->getContext()),
mTestFunctionCall(nullptr),
mCurrentBB(nullptr),
mReturnValue(nullptr),
mWarnings()
{
}

/**
 * Create the appropriate instructions for each argument type
 */
void TestGeneratorVisitor::VisitFunctionArgument(tp::FunctionArgument *arg)
{
	FunctionCall* parent = arg->getParent();
	string func_name = parent->getIdentifier()->getIdentifierStr();
	llvm::Function *currentFunction = mModule->getFunction(func_name);
	if( currentFunction == nullptr) {
		cout << "Function not found!: " << func_name << endl;
	}
	assert(currentFunction != nullptr && "Function not found!");
	llvm::Function::arg_iterator arg_it = currentFunction->arg_begin();

	unsigned i = 0;
	while (arg_it != currentFunction->arg_end()) {
		if (i == arg->getIndex()) {
			llvm::Argument& llvm_arg = *arg_it;

			if (llvm_arg.getType()->getTypeID() == Type::TypeID::PointerTyID) {
				// this is a buffer allocation
				if(BufferAlloc *ba = arg->getBufferAlloc()) {
					assert(ba != nullptr && "Invalid BufferAlloc pointer");
					AllocaInst *alloc1 = bufferAllocInitialization(llvm_arg.getType(),ba, mInstructions);

					// Allocate memory for a pointer type
					AllocaInst *alloc2 = mBuilder.CreateAlloca(llvm_arg.getType(), 0, "AllocPtr" + Twine(i)); // Allocate a pointer type
					StoreInst *store2 = mBuilder.CreateStore(alloc1, alloc2); // Store an already allocated variable address to our pointer
					LoadInst *load = mBuilder.CreateLoad(alloc2, "value3"); // Load whatever address is in alloc3 into load3 'value3'

					mInstructions.push_back(alloc2);
					mInstructions.push_back(store2);
					mInstructions.push_back(load);
					mArgs.push_back(load);
				} else {
					const tp::Argument* my_string = arg->getArgument();
					const TokenType type = my_string->getTokenType();
					if(type == TOK_STRING) {
						const string& my_str = my_string->getStringRepresentation();
						ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(mModule->getContext(), 8), my_str.size()+1); // + null
						GlobalVariable* gvar_array__str = new GlobalVariable(/*Module=*/*mModule,
							   /*Type=*/ArrayTy_0,
							   /*isConstant=*/true,
							   /*Linkage=*/GlobalValue::PrivateLinkage,
							   /*Initializer=*/0, // has initializer, specified below
							   /*Name=*/".str");
						gvar_array__str->setAlignment(1);
						llvm::Constant *array_str = ConstantDataArray::getString(mModule->getContext(), my_str, true);
						gvar_array__str->setInitializer(array_str);

						std::vector<llvm::Constant*> indices;
						ConstantInt* zero = ConstantInt::get(mModule->getContext(), APInt(32, StringRef("0"), 10));
						indices.push_back(zero);
						indices.push_back(zero);
						llvm::Constant *str_ptr = ConstantExpr::getGetElementPtr(gvar_array__str, indices);

						mArgs.push_back(str_ptr);
					} else {
						const string& my_str = my_string->getStringRepresentation();
						// We will only allow initializing to 0 (nullptr) for the cases
						// in which the users want to test the flow for handling nullptrs,
						// however we may remove this 'feature' if it causes too much
						// trouble.
						if(type == TOK_INT and my_str == "0") {
							PointerType * ptrType = dyn_cast<PointerType>(llvm_arg.getType());
							mArgs.push_back(ConstantPointerNull::get(ptrType));
							break;
						}
						// Any other integer or float constant cannot be used to initialize
						// a pointer.
						/// @todo Get the info on which line and column this token is located at
						throw Exception(0,0,"Cannot initialize pointer with arbitrary address "+my_str,
							"Use the syntax [n:x] so I can allocate the memory for you.");
					}
				}
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
	if( funcToBeCalled == nullptr) {
		cout << "Function not found!: " << func_name << endl;
	}
	assert(funcToBeCalled != nullptr && "Function not found!");
	CallInst *call = mBuilder.CreateCall(funcToBeCalled, mArgs);

	if (funcToBeCalled->getReturnType()->getTypeID() == Type::TypeID::VoidTyID)
		mReturnValue = mBuilder.getInt32(0); // for void types always return 0
	else
		mReturnValue = call;

	FC->setReturnType(call->getCalledFunction()->getReturnType());
	mTestFunctionCall = call;
	mInstructions.push_back(call);
	mArgs.clear();
}

void TestGeneratorVisitor::VisitExpectedResult(ExpectedResult *ER)
{
	// Get the call instruction pushed by VisitFunctionCall or the last instructions
	// pushed by this same method VisitExpectedResult
    CallInst *call = dyn_cast<CallInst>(mInstructions.back());
    if(call == nullptr)
        throw Exception("Invalid CallInst!");

	Type* returnedType = call->getCalledFunction()->getReturnType();
	if(returnedType->getTypeID() == Type::TypeID::VoidTyID) {
		stringstream ss;
		ss<<"Trying compare a value against a function with no return value (void)"<<endl;
		mWarnings.push_back(Exception(ss.str(), "", Exception::WARNING));
		return;
	}

	stringstream ss;
	tp::Constant* EC = ER->getExpectedConstant()->getConstant();
	if(EC->isCharConstant()) {
		ss << EC->getValue();
	}

	if(EC->isNumericConstant()) {
		ss << ER->getExpectedConstant()->getConstant()->getAsStr();
		assert(ss.str().size() && "Invalid numeric string!");
	}

	if(EC->isStringConstant()) {
		const string& str = ER->getExpectedConstant()->getConstant()->getStringConstant()->getString();
		stringstream tmp;
		tmp << static_cast<const void*>(str.c_str());
		unsigned long addr =  stoul(tmp.str(), nullptr, 16);
		ss << addr;
		assert(ss.str().size() && "Invalid string constant!");
	}

	llvm::Value* c = createValue(returnedType, ss.str());

	string InstName = "ComparisonInstruction";
    Value* i = nullptr;
	if (returnedType->getTypeID() == Type::IntegerTyID) {
		i = createIntComparison(ER->getComparisonOperator()->getType(), call, c);
	}
	else if (returnedType->getTypeID() == Type::FloatTyID or
			returnedType->getTypeID() == Type::DoubleTyID) {
		i = createFloatComparison(ER->getComparisonOperator()->getType(), call, c);
	}
	else
		assert(false && "Unsupported type for comparison operator!");

	assert(i && "Invalid comparison instruction");
	mInstructions.push_back((llvm::Instruction*)i);
	// convert an i1 type to an i8 type for proper comparison to bool.
	llvm::ZExtInst* zext = (llvm::ZExtInst*) mBuilder.CreateZExt(i,mBuilder.getInt8Ty());
	mInstructions.push_back(zext);
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
			ss <<  LHS->getConstant()->getAsStr();
			L = createValue(R->getType(),ss.str());
		}
	}

	if (RHS->isConstant()) {
		if (L != nullptr) {
			stringstream ss;
			ss <<  RHS->getConstant()->getAsStr();
			R = createValue(L->getType(),ss.str());
		}
	}

	string InstName = "ExpExprComp";
	Value* i = nullptr;
    switch(L->getType()->getTypeID()) {
		case Type::PointerTyID: // Workaround, keep an eye on this case for pointers
		case Type::IntegerTyID:
			i = createIntComparison(CO->getType(),L,R);
			break;
		case Type::FloatTyID:
			i = createFloatComparison(CO->getType(),L,R);
			break;
		case Type::DoubleTyID:
			i = createFloatComparison(CO->getType(),L,R);
			break;
		default:
			assert(false && "Invalid case not supported");
	}
	assert(i && "Invalid ComparisonOperator");

	// It means this is an ExpectedExpression in a unit test (NOT in GlobalTeardown)
	if(mReturnValue != nullptr) {
		llvm::ZExtInst* zext = (llvm::ZExtInst*) mBuilder.CreateZExt(i,mReturnValue->getType());
		// We perform this comparison for the cases in which we called a unit test
		// (function) and we use its return value to chain it with the rest of
		// comparisons.
		llvm::Value* mainBool = mBuilder.CreateICmpEQ(mReturnValue, zext);
		llvm::ZExtInst* zext2 = (llvm::ZExtInst*) mBuilder.CreateZExt(mainBool,mReturnValue->getType());

		mInstructions.push_back((llvm::Instruction*)i);
		mInstructions.push_back(zext);
		mInstructions.push_back((llvm::Instruction*)mainBool);
		mInstructions.push_back(zext2);
		mReturnValue = zext2;
	} else {
		// It means we are in a GlobalTeardown or GlobalSetup because mReturnValue
		// is never assigned when processing before_all or after_all, so it always
		// remains nullptr
		llvm::ZExtInst* zext = (llvm::ZExtInst*) mBuilder.CreateZExt(i,mBuilder.getInt32Ty());// We know we always return int32
		mInstructions.push_back((llvm::Instruction*)i);
		mInstructions.push_back(zext);
		mReturnValue = zext;
	}
}

void TestGeneratorVisitor::VisitMockupFunction(MockupFunction* MF)
{
	FunctionCall* FC = MF->getFunctionCall();
	string func_name = FC->getIdentifier()->getIdentifierStr();
	string mockup_name = func_name+ "_mockup";

	while(mModule->getFunction(mockup_name)) {
		stringstream ss;
		ss <<"_";
		ss << mMockupNames.size();
		mockup_name = mockup_name + ss.str();
	}
	mMockupNames[mockup_name] = true;
	llvm::Function *llvm_func = mModule->getFunction(func_name);
	if( llvm_func == nullptr) {
		cout << "Function NOT found...creating declaration and definition" << endl;
		// If no declaration is found we need to figure out the return type somehow
		assert(false && "TODO: Implement this!");
	} else {
		// Function definition or declaration
		cout << "Function found: " << func_name << endl;

		tp::Argument* expected = MF->getArgument();
		vector<Type*> params;
		for(llvm::Argument& arg : llvm_func->getArgumentList()) {
			params.push_back(arg.getType());
		}

		FunctionType* FT = FunctionType::get(llvm_func->getReturnType(),ArrayRef<Type*>(params),false);
		Function* F = cast<Function>(mModule->getOrInsertFunction(mockup_name, FT, llvm_func->getAttributes()));

		// create unique name
		BasicBlock* MB = BasicBlock::Create(mModule->getContext(),"mockup_block",F);
		// The expected value has to match the return value type from the llvm_func
		llvm::Value* val = createValue(llvm_func->getReturnType(), expected->getStringRepresentation());
		ReturnInst* ret = mBuilder.CreateRet(val);
		MB->getInstList().push_back(ret);
			/// WE WILL NEED TO CREATE A NEW FUNCTION EACH TIME BECAUSE OF THE
			/// WAY THE VISITOR PATTERN IS IMPLEMENTED.
			/// WE CREATE THE FUNCTIONS FIRST, THEN CALL THEM
			/// THIS MEANS WE CANNOT WORK ON THE SAME FUNCTION OVER AND OVER AGAIN.
			/// WE NEED TO BORROW IT'S ORIGINAL DECLARATION OR DEFINITION
			/// CREATE A NEW NAME, STORE IN tp::Argument AND THEN CREATE A FUNCTION
			/// WITH THAT NAME. THUS WE'LL HAVE TO REPLACE THE CALL OF EACH OF
			/// THE ORIGINAL FUNCTIONS WITH A CALL TO A MOCKUP
		// this line does what I want, but I need to understand how to change it back.

		MF->setMockupFunction(F);
		MF->setOriginalFunction(llvm_func);
	}
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
	TokenType tokenType = VA->getTokenType();
	string real_value;

	if(VA->getArgument()) {
		real_value = VA->getArgument()->getStringRepresentation();
	}

	LoadInst* load_value = mBuilder.CreateLoad(global_variable);
	mInstructions.push_back(load_value);

        GlobalVariable* backup = new GlobalVariable(*mModule,
                        global_variable->getType()->getElementType(),
                        false,
                        GlobalValue::LinkageTypes::ExternalLinkage,
                        0,
                        "backup_"+global_variable->getName().str(),
                        global_variable);
        backup->setInitializer(global_variable->getInitializer());
        backup->copyAttributesFrom(global_variable);

        StoreInst* st = mBuilder.CreateStore(load_value, backup);
        mInstructions.push_back(st);

        mBackupTemp.push_back(make_tuple(backup,global_variable));
	// TODO: Handle the rest of token types
	switch (tokenType) {
		case TOK_INT:
		case TOK_FLOAT:
		{
			// Watch for the real type of a global variable, there might be a @bug
			Value * v = createValue(global_variable->getType()->getPointerElementType(), real_value);
			StoreInst *store = mBuilder.CreateStore(v, global_variable);
			mInstructions.push_back(store);
		}
			break;
		case TOK_STRUCT_INIT:
		{
			StructInitializer* str_init = VA->getStructInitializer();
			vector<Value*> values;
			values.push_back(mBuilder.getInt32(0));
			extractInitializerValues(global_variable, str_init,&values, mInstructions);
		}
			break;
		case TOK_BUFF_ALLOC:
		{
			tp::BufferAlloc* ba = VA->getBufferAlloc();
			// global_variable is a pointer to a pointer.
			AllocaInst* alloc = bufferAllocInitialization(global_variable->getType()->getElementType(), ba, mPtrAllocation);
			StoreInst *store2 = mBuilder.CreateStore(alloc, global_variable); // Store an already allocated variable address to our pointer
			// alloc was already pushed inside bufferAllocInitialization
			mPtrAllocation.push_back(store2);
		}
		break;
		default:
			assert(false && "Unhandled case! FIX ME");
			break;
	}
}

void TestGeneratorVisitor::VisitTestDefinitionFirst(TestDefinition *TD)
{
    mCurrentFud = TD->getTestFunction()->getFunctionCall()->getIdentifier()->getIdentifierStr();
}

void TestGeneratorVisitor::VisitTestSetup(TestSetup *TS)
{
    string func_name = "setup_"+mCurrentFud;
    Function *testFunction = generateFunction(func_name, true);
    TS->setLLVMFunction(testFunction);

    // Any variable assignment done in the current test pass it to the vector for
    // the current test.
    for(auto& b : mBackupTemp)
        mBackupTest.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}

void TestGeneratorVisitor::VisitTestFunction(TestFunction *TF)
{
    string func_name = "test_" + mCurrentFud;
	mInstructions.insert(mInstructions.begin(), mPtrAllocation.begin(), mPtrAllocation.end());
	mPtrAllocation.clear();

	/// Create the global variable that will hold the bool value for the comparison
	/// between the actual result and the expected result.
	GlobalVariable* result = new GlobalVariable(*mModule,
					mBuilder.getInt8Ty(),
					false,
					GlobalValue::LinkageTypes::ExternalLinkage,
					mBuilder.getInt8(1)); // Providing an initializer 'DEFINES' the variable
	// If zext == nullptr no expected expression was provided and we assume
	// the test passed.
	if(llvm::ZExtInst* zext = dyn_cast<ZExtInst>(mInstructions.back())) {
		// if zext != nullptr means that there was an expected expression and
		// we should store the bool result in the global variable we just created.
		StoreInst* st = mBuilder.CreateStore(zext, result);
		mInstructions.push_back(st);
	}

    Function *testFunction = generateFunction(func_name,true);
	result->setName("result_"+testFunction->getName());
	TF->setResultVariable(result);
    TF->setLLVMFunction(testFunction);
}

void TestGeneratorVisitor::VisitTestTeardown(TestTeardown *TT)
{
    string func_name = "teardown_"+mCurrentFud;
    mInstructions.insert(mInstructions.begin(), mPtrAllocation.begin(), mPtrAllocation.end());
    mPtrAllocation.clear();
    Function *testFunction = generateFunction(func_name, true);
    TT->setLLVMFunction(testFunction);

    // Any variable assignment done in the current test pass it to the vector for
    // the current test
    for(auto& b : mBackupTemp)
        mBackupTest.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}

void TestGeneratorVisitor::VisitTestDefinition(TestDefinition *TD)
{
    if(mBackupTest.size() ) {
        string func_name = "cleanup_test_"+TD->getTestFunction()->getFunctionCall()->getIdentifier()->getIdentifierStr();
        restoreGlobalVariables(mBackupTest);
        Function *testFunction = generateFunction(func_name);
        TD->setLLVMFunction(testFunction);
    }
    // The warnings may include test-setup, test-function, or test-teardown
    TD->setWarnings(mWarnings);

    mWarnings.clear();
}

void TestGeneratorVisitor::VisitTestGroupFirst(TestGroup *)
{
}

void TestGeneratorVisitor::VisitGlobalSetup(GlobalSetup *GS)
{
    string func_name = "group_setup_"+GS->getGroupName();
    Function *testFunction = generateFunction(func_name, true);
    GS->setLLVMFunction(testFunction);

    // Any variable assignment done in the current group pass it to the vector for
    // the current group.
    for(auto& b : mBackupTemp)
        mBackupGroup.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}

void TestGeneratorVisitor::VisitGlobalTeardown(GlobalTeardown *GT)
{
    string func_name = "group_teardown_"+GT->getGroupName();
    Function *testFunction = generateFunction(func_name, true);
    GT->setLLVMFunction(testFunction);

    // Any variable assignment done in the current group pass it to the vector for
    // the current group.
    for(auto& b : mBackupTemp)
        mBackupGroup.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}

void TestGeneratorVisitor::VisitTestGroup(TestGroup *TG)
{
    if(mBackupGroup.size()) {
        string func_name = "group_cleanup_"+TG->getGroupName();
        restoreGlobalVariables(mBackupGroup);
        Function *cleanupFUnction = generateFunction(func_name);
        TG->setLLVMFunction(cleanupFUnction);
    }
}

/**
 * Creates a new Value of the same Type as type with real_value
 *
 * @TODO Improve the way this method works and its documentation
 */
llvm::Value* TestGeneratorVisitor::createValue(llvm::Type* type,
		const string& real_value)
{
	Type::TypeID typeID = type->getTypeID();
	switch (typeID) {
	case Type::TypeID::IntegerTyID:
	{
		string value = real_value;
		if(real_value.find('.') != string::npos) {
			value = real_value.substr(0, real_value.find('.'));
			stringstream ss;
			ss << "Casting floating point value "<<real_value<<" to "<<value;
			mWarnings.push_back(Exception(ss.str(), "", Exception::WARNING));
		}

		if (IntegerType * intType = dyn_cast<IntegerType>(type))
			// Watch for the radix, right now we use radix 10 only
			return cast<Value>(mBuilder.getInt(APInt(intType->getBitWidth(), value, 10)));
	}
		break;
	// Even though this might look a repeated case like DoubleTyID, I will just
	// leave it here to remember that I need to do more research on how LLVM API
	// handles floating point values
	case Type::TypeID::FloatTyID:
	{
		llvm::Constant* c = ConstantFP::get(type, real_value);
		return c;
	}
		break;
	case Type::TypeID::DoubleTyID:
	{
		llvm::Constant* c = ConstantFP::get(type, real_value);
		return c;
	}
		break;
	case Type::TypeID::PointerTyID:
		if (PointerType * ptrType = dyn_cast<PointerType>(type)) {
			return ConstantPointerNull::get(ptrType);
		}
	case Type::TypeID::VoidTyID:
		assert(false && "Cannot create a void value!");
	default:
		cerr << "Could not create value for typeID: " << typeID << endl;
		return nullptr;
	}
	return nullptr;
}

llvm::Function* TestGeneratorVisitor::generateFunction(const string& name,
                                                        bool use_mReturnValue)
{
    string unique_name = getUniqueTestName(name);
	llvm::Type* return_type = nullptr;
	if(mTestFunctionCall)
		return_type = mTestFunctionCall->getCalledFunction()->getReturnType();
	else
		return_type = mBuilder.getInt8Ty();
	Function *function = cast<Function> (mModule->getOrInsertFunction(
			unique_name,
			return_type,
			nullptr));
	BasicBlock *BB = BasicBlock::Create(mModule->getContext(),
			"block_" + unique_name, function);

	ReturnInst *ret = nullptr;
	if(use_mReturnValue && mReturnValue) {
		ret = mBuilder.CreateRet(mReturnValue);
	}
	else
            ret = mBuilder.CreateRet(mBuilder.getInt32(1));// the setup/teardown passed

	mInstructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : mInstructions)
		mBuilder.Insert(inst);

	mInstructions.clear();
	mBackup.clear();
	mBuilder.ClearInsertionPoint();
	mReturnValue = nullptr;

	return function;
}

void TestGeneratorVisitor::saveGlobalVariables()
{
	llvm::GlobalVariable* dst = nullptr;
	mBackupGroup.push_back(make_tuple(nullptr,nullptr));
	// Get the values of the global variables that were to be modified.
	// Then create new global variables and assign them the original values
	// from the global variables.
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
	mBackup.clear();
}

void TestGeneratorVisitor::restoreGlobalVariables(std::vector<tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>>& backup)
{
	// Restore the backed up variables from the global variables created in
	// VisitGlobalSetup
	// @bug This works only where we declare a teardown with "after_all"
	while(backup.size()) {
		tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>& tup = backup.back();
		backup.pop_back();
		if(get<0>(tup)==nullptr and get<1>(tup)==nullptr)
			break;
		LoadInst* ld = mBuilder.CreateLoad(get<0>(tup));
		StoreInst* st = mBuilder.CreateStore(ld, get<1>(tup));
		mInstructions.push_back(ld);
		mInstructions.push_back(st);
	}
}

void TestGeneratorVisitor::extractInitializerValues(Value* global_struct,
		const StructInitializer* init,
		vector<Value*>* ndxs,
                std::vector<llvm::Instruction*>& instructions)
{
	if (init->getInitializerList()) {
		InitializerList* init_list = init->getInitializerList();
		const vector<tp::InitializerValue*>& args = init_list->getArguments();
		int i = 0;
		for(tp::InitializerValue* arg : args) {
			ndxs->push_back(mBuilder.getInt32(i));
			ArrayRef<Value*> arr(*ndxs);
			GetElementPtrInst* gep =
					static_cast<GetElementPtrInst*> (
					mBuilder.CreateGEP(global_struct,
					arr,
					"struct_"+Twine(i))
					);

			// @bug Add only if gep instruction is not already in the container.
			// @todo investigate why is gep instruction already in a container
			if(gep->getParent() == nullptr)
				instructions.push_back(gep);

			string str_value;
			if (arg->isArgument()) {
				str_value = arg->getArgument().getStringRepresentation();
				Value* v = createValue(gep->getType()->getPointerElementType(), str_value);
				StoreInst* store = mBuilder.CreateStore(v,gep);
				instructions.push_back(store);
			}
			else if (arg->isStructInitializer()) {
				const StructInitializer& sub_init = arg->getStructInitializer();
				extractInitializerValues(global_struct,&sub_init,ndxs,instructions);
			}
			i++;
			ndxs->pop_back();
		}
	} else {
		/// @note The reason why designated initializers are not supported
		/// is because LLVM API does not save the field names of a struct.
		/// The only way to access is through indexes, thus finding the index
		/// based on the name is impossible using LLVM API.
		/// We could use clang API, however I made a quick search and seems
		/// we'll hit the same problem.
		/// The only solution to this is to parse the source code ourselves and
		/// find the struct definitions, and extract their names and create a
		/// data structure that we can use at this pointer.
		/// Implementing this feature will take a lot of time, so I decided
		/// to drop it for the first prototype.
		assert(false && "Designated initializers are not supported!");
		DesignatedInitializer* des_init = init->getDesignatedInitializer();
		const vector<tuple<Identifier*,InitializerValue*>>& args = des_init->getInitializers();
		// find the index for a given identifier
		Identifier* id = nullptr;
		InitializerValue* init_val = nullptr;
		for(const tuple<Identifier*,InitializerValue*>& tup : args) {
			id = get<0>(tup);
			init_val = get<1>(tup);
			id->dump();
			cout << " = ";
			init_val->dump();
			cout << endl;
			//global_struct->dump();
			Type* struct_type = global_struct->getType()->getPointerElementType();
			cout<<"Num of contained types: "<<struct_type->getNumContainedTypes()<<endl;
			cout<<"getStructNumElements(): "<<struct_type->getStructNumElements()<<endl;
			cout<<"Struct name: "<<struct_type->getStructName().str()<<endl;
			cout<<"TypeID: "<<struct_type->getTypeID()<<endl;
			if (struct_type->isStructTy()) {
				cout<<"This is a struct type!"<<endl;
				for(Type::subtype_iterator it = struct_type->subtype_begin();
						it != struct_type->subtype_end(); ++it) {
					Type* t = *it;
					cout<<"SUBTYPE TYPE: "<<t->getTypeID()<<endl;
				}
			}
			cout << endl;
		}
	}
}

Value* TestGeneratorVisitor::createIntComparison(ComparisonOperator::Type type,
										llvm::Value* LHS, llvm::Value* RHS)
{
	string InstName = "IntComparison";
	switch(type) {
        case ComparisonOperator::EQUAL_TO:
			return mBuilder.CreateICmpEQ(LHS, RHS, InstName);
        case ComparisonOperator::NOT_EQUAL_TO:
			return mBuilder.CreateICmpNE(LHS, RHS, InstName);
        case ComparisonOperator::GREATER_OR_EQUAL:
			return mBuilder.CreateICmpSGE(LHS, RHS, InstName);
        case ComparisonOperator::LESS_OR_EQUAL:
			return mBuilder.CreateICmpSLE(LHS, RHS, InstName);
        case ComparisonOperator::GREATER:
			return mBuilder.CreateICmpSGT(LHS, RHS, InstName);
        case ComparisonOperator::LESS:
			return mBuilder.CreateICmpSLT(LHS, RHS, InstName);
        case ComparisonOperator::INVALID:
			return nullptr;
    }
	return nullptr;
}

Value* TestGeneratorVisitor::createFloatComparison(ComparisonOperator::Type type,
										llvm::Value* LHS, llvm::Value* RHS)
{
	string InstName = "FloatComparison";
	switch(type) {
        case ComparisonOperator::EQUAL_TO:
			return mBuilder.CreateFCmpOEQ(LHS, RHS, InstName);
        case ComparisonOperator::NOT_EQUAL_TO:
			return mBuilder.CreateFCmpONE(LHS, RHS, InstName);
        case ComparisonOperator::GREATER_OR_EQUAL:
			return mBuilder.CreateFCmpOGE(LHS, RHS, InstName);
        case ComparisonOperator::LESS_OR_EQUAL:
			return mBuilder.CreateFCmpOLE(LHS, RHS, InstName);
        case ComparisonOperator::GREATER:
			return mBuilder.CreateFCmpOGT(LHS, RHS, InstName);
        case ComparisonOperator::LESS:
			return mBuilder.CreateFCmpOLT(LHS, RHS, InstName);
        case ComparisonOperator::INVALID:
			return nullptr;
    }
	return nullptr;
}

llvm::AllocaInst* TestGeneratorVisitor::bufferAllocInitialization(llvm::Type* ptrType, tp::BufferAlloc *ba,
	std::vector<llvm::Instruction*>& instructions)
{
	assert(ptrType && "Invalid ptrType");
	assert(ptrType->getTypeID() == Type::PointerTyID && "ptrType has to be a pointer type");
	assert(ptrType->getPointerElementType()->getTypeID() != Type::FunctionTyID && "Pointers to functions not supported");
	AllocaInst *alloc1 = mBuilder.CreateAlloca(
						ptrType->getPointerElementType(), // @note watch for bug here when type is not a pointer type
						mBuilder.getInt(APInt(32, ba->getBufferSizeAsString(), 10)) // @todo Add support to hex and octal bases
						); // Allocate memory for the element type pointed to

	instructions.push_back(alloc1);

	if(ptrType->getPointerElementType()->getTypeID() == Type::TypeID::StructTyID) {
		////////////////////////////////////////
		// This code is used to initialize the memory allocated for the
		// local struct to 0. If there is an initializer specified, it will
		// overwrite the 0s, which is what we want.
		DataLayout dl(mModule->getDataLayout());
		uint64_t size = dl.getTypeAllocSize(ptrType->getPointerElementType());
		assert(size && "Invalid structure size");
		Value* bitcast = mBuilder.CreateBitCast(alloc1,mBuilder.getInt8PtrTy());
		assert(bitcast && "Invalid bitcast instruction");
		ConstantInt* zero_value = mBuilder.getInt8(0);
		instructions.push_back(cast<Instruction>(bitcast));
		for(unsigned i = 0; i < size * ba->getBufferSize(); i++) {
			GetElementPtrInst* gep = cast<GetElementPtrInst>(mBuilder.CreateGEP(bitcast,mBuilder.getInt64(i)));
			StoreInst* store = mBuilder.CreateStore(zero_value, gep);
			instructions.push_back(gep);
			instructions.push_back(store);
		}
		// End of initialization.
		/////////////////////////////////////////

		if(ba->isAllocatingStruct()) {
			const StructInitializer* init = ba->getStructInitializer();
			for(unsigned i=0; i < ba->getBufferSize(); i++) {
				GetElementPtrInst* gep = cast<GetElementPtrInst>(mBuilder.CreateGEP(bitcast,mBuilder.getInt64(i*size)));
				instructions.push_back(gep);
				BitCastInst* bc = cast<BitCastInst>(mBuilder.CreateBitCast(gep,ptrType));
				instructions.push_back(bc);
				Value* val = cast<Value>(bc);
				assert(val && "Invalid Value type");
				vector<Value*> indices;
				indices.push_back(mBuilder.getInt32(0));
				extractInitializerValues(val, init, &indices, instructions);
			}
		}

	} else {
		// @note watch for bug here when type is not a pointer type
		Value *v = createValue(ptrType->getPointerElementType(), ba->getDefaultValueAsString());
		// Initialize the whole buffer to the default value
		// at the moment I didn't come up with a better idea other than
		// iterate over the whole buffer.
		for(unsigned i = 0; i < ba->getBufferSize(); i++) {
			Value* gep = mBuilder.CreateGEP(alloc1,	mBuilder.getInt32(i), Twine("array_"+i));
			instructions.push_back(static_cast<llvm::Instruction*>(gep));
			StoreInst *S = mBuilder.CreateStore(v, gep);
			instructions.push_back(S);
		}
	}
	return alloc1; //alloc1 is a pointer type to type.
}

string TestGeneratorVisitor::getUniqueTestName(const string& name)
{
    string unique_name = name + "_0";
    unsigned i = 0;

    do {
            unsigned pos = unique_name.find_last_of("_");
            unsigned discard = unique_name.size() - (++pos);
            while(discard--) unique_name.pop_back();
            stringstream ss;
            ss << i;
            unique_name = unique_name + ss.str();
            ++i;
    } while (mModule->getFunction(unique_name));

    return unique_name;
}