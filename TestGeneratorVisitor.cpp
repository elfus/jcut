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
mFUDReturnValue(nullptr),
mWarnings(),
mTestResult(nullptr)
{
}

/**
 * Create the appropriate instructions for each argument type
 */
void TestGeneratorVisitor::VisitFunctionArgument(tp::FunctionArgument *arg)
{
	if(arg->isDataPlaceholder()) {
		string file = __FILE__;
		unsigned pos = file.find_last_of('/') + 1;
		file = file.substr(pos, file.size() - pos);
		Exception::mCurrentFile = file;
		throw Exception(__LINE__,0,"DataPlaceholders are not allowed inside a before or after statement");
	}

	llvm::Function *currentFunction = mModule->getFunction(mCurrentFuncCall);
	if( currentFunction == nullptr) {
		cout << "Function not found!: " << mCurrentFuncCall << endl;
	}
	assert(currentFunction != nullptr && "Function not found!");
	llvm::Function::arg_iterator arg_it = currentFunction->arg_begin();

	unsigned i = 0;

	while (arg_it != currentFunction->arg_end()) {
		if (i == arg->getIndex()) {
			llvm::Argument& llvm_arg = *arg_it;
			if (llvm_arg.getType()->getTypeID() == Type::TypeID::PointerTyID) {
				// this is a buffer allocation
				if(const BufferAlloc *ba = arg->getBufferAlloc()) {
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
					const tp::Constant* my_string = arg->getArgument();
					const TokenType type = my_string->getTokenType();
					Type * ptrElementType = llvm_arg.getType()->getPointerElementType();
					// We can only use strings when pointing to a char (8 bits)
					if(type == TOK_STRING and ptrElementType == mBuilder.getInt8Ty() ) {
						const string& quoted = my_string->toString();
						// Remove the quotes "
						const string my_str = quoted.substr(1,quoted.size()-2);
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
						break;
					}

					if(type == TOK_STRING and ptrElementType != mBuilder.getInt8Ty() ) {
						const string& quoted = my_string->toString();
						/// @todo Get the info on which line and column this token is located at
						throw Exception(0,0,"Cannot initialize pointer with string constant "+quoted,
							"'char *' or 'unsigned char *' can only be initialized with string constants.");
					}

					if(type == TOK_INT || type == TOK_FLOAT)
					{
						const string& my_str = my_string->getNumericConstant()->toString();
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
						throw Exception("Cannot initialize pointer with arbitrary address "+my_str+
							"\nUse the syntax [n:x] so I can allocate the memory for you.");
					}
				}
				break;
			}
			if(arg->isBufferAlloc())
				throw Exception(arg->getLine(), arg->getColumn(),
						"Invalid argument type [BufferAlloc] for function "+mCurrentFuncCall);
			string str_value;
			// @todo Improve the way we detect when we have to get the memory address
			// from a string. This is only a quick patch which I need right now :(
			if(arg->getArgument()->getTokenType() == TOK_STRING) {
				stringstream ss;
				const string& the_string =  arg->getArgument()->toString();
				char *c = const_cast<char*>(&(the_string.c_str())[0]);
				ss << reinterpret_cast<void*>(c);
				str_value = ss.str();
				mWarnings.push_back(
						Exception("Incompatible pointer conversion.",
								"Passing pointer address "+str_value+" of string "+the_string+""
								" to parameter of non-pointer type ", Exception::WARNING));
			} else if(arg->getArgument()->getTokenType() == TOK_CHAR) {
				stringstream ss;
				ss << static_cast<int>(arg->getArgument()->getCharConstant()->getChar());
				str_value = ss.str();
			} else
				str_value = arg->toString();
			// Code for non-pointer types
			llvm::AllocaInst *alloc = mBuilder.CreateAlloca(llvm_arg.getType(), 0, "Allocation" + Twine(i));
			alloc->setAlignment(4);
			// The type is an IntegerType so we have to make sure that
			// whatever data is hold in str_value it, string or char, must be
			// its equivalent to an integer value.
			Value *v = createValue(llvm_arg.getType(), str_value);
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

void TestGeneratorVisitor::VisitFunctionCallFirst(FunctionCall *FC) {
	mCurrentFuncCall = FC->getIdentifier()->toString();
}

void TestGeneratorVisitor::VisitFunctionCall(FunctionCall *FC)
{
	string func_name = FC->getIdentifier()->toString();
	Function *funcToBeCalled = nullptr;

	auto it = mFunctionsWrapped.find(func_name);
	if(it != mFunctionsWrapped.end())
		funcToBeCalled = mFunctionsWrapped[func_name];
	else
		funcToBeCalled = mModule->getFunction(func_name);

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
	mCurrentFuncCall.clear();
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
	ExpectedConstant* EC = ER->getExpectedConstant();
	assert(EC->isDataPlaceholder() == false && "Cannot generate ExpectedResult code from a DataPlaceholder.");

	tp::Constant* C = EC->getConstant();
	if(C->isCharConstant()) {
		ss << C->getValue();
	}

	if(C->isNumericConstant()) {
		ss << EC->getConstant()->toString();
		assert(ss.str().size() && "Invalid numeric string!");
	}

	if(C->isStringConstant()) {
		const string& str = EC->getConstant()->getStringConstant()->getString();
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
	mTestResult = zext;
}

void TestGeneratorVisitor::VisitExpectedExpression(ExpectedExpression *EE)
{
	Operand* LHS = EE->getLHSOperand();
	Operand* RHS = EE->getRHSOperand();
	ComparisonOperator* CO = EE->getComparisonOperator();
	Value* L = nullptr;
	Value* R = nullptr;

	if (LHS->isIdentifier()) {
		llvm::GlobalVariable* g = mModule->getGlobalVariable(LHS->getIdentifier()->toString());
		assert(g && "LHS Operator not found!");
		L = mBuilder.CreateLoad(g);
		mInstructions.push_back((llvm::Instruction*)L);
	}

	if (RHS->isIdentifier()) {
		llvm::GlobalVariable* g = mModule->getGlobalVariable(RHS->getIdentifier()->toString());
		assert(g && "RHS Operator not found!");
		R = mBuilder.CreateLoad(g);
		mInstructions.push_back((llvm::Instruction*)R);
	}

	if (LHS->isConstant()) {
		if (R != nullptr) {
			stringstream ss;
			ss <<  LHS->getConstant()->toString();
			L = createValue(R->getType(),ss.str());
		}
	}

	if (RHS->isConstant()) {
		if (L != nullptr) {
			stringstream ss;
			ss <<  RHS->getConstant()->toString();
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

	mInstructions.push_back((llvm::Instruction*)i);

	llvm::ZExtInst* return_value = (llvm::ZExtInst*) mBuilder.CreateZExt(i,mBuilder.getInt8Ty());
	mInstructions.push_back((llvm::Instruction*)return_value);

	/// Create the global variable that will hold the bool value for the comparison
	/// between the actual result and the expected result.
	GlobalVariable* result = new GlobalVariable(*mModule,
					mBuilder.getInt8Ty(),
					false,
					GlobalValue::LinkageTypes::ExternalLinkage,
					mBuilder.getInt8(1),
					"expected_expression_"+mCurrentFud); // Providing an initializer 'DEFINES' the variable
	llvm::Instruction* res = cast<llvm::Instruction>(mBuilder.CreateStore(return_value, result));
	mInstructions.push_back(res);
	EE->setGlobalVariable(result);
}

void TestGeneratorVisitor::VisitMockupFunction(MockupFunction* MF)
{
	FunctionCall* FC = MF->getFunctionCall();
	string func_name = FC->getIdentifier()->toString();
	string mockup_name = func_name+ "_mockup_0";

	while(mModule->getFunction(mockup_name)) {
		stringstream ss;
		ss <<"_";
		ss << mMockupNames.size();
		mockup_name = mockup_name.substr(0, mockup_name.find_last_of('_')) + ss.str();
	}
	mMockupNames[mockup_name] = true;
	llvm::Function *llvm_func = mModule->getFunction(func_name);
	if( llvm_func == nullptr) {
		cout << "Function NOT found...creating declaration and definition" << endl;
		// If no declaration is found we need to figure out the return type somehow
		assert(false && "TODO: Implement this!");
	} else {
		tp::Constant* expected = MF->getArgument();
		if(llvm_func->getReturnType() == mBuilder.getVoidTy() &&
			expected->toString() != "void") {
			throw Exception("The function "+func_name+"() has void as return value. ",
					"The only valid syntax for a void function is: "+func_name+"() = void;",
					Exception::ERROR);
		}
		///////////////////////////////////////////////////////
		// Create the mockup function which will return whatever the user
		// defined in the test file (mockup {} statement)
		vector<Type*> params;
		for(llvm::Argument& arg : llvm_func->getArgumentList())
			params.push_back(arg.getType());
		FunctionType* FT = FunctionType::get(llvm_func->getReturnType(),ArrayRef<Type*>(params),false);
		Function* mockup_function = cast<Function>(mModule->getOrInsertFunction(mockup_name, FT, llvm_func->getAttributes()));
		BasicBlock* MB = BasicBlock::Create(mModule->getContext(),"mockup_block",mockup_function);
		// The expected value has to match the return value type from the llvm_func
		ReturnInst* ret = nullptr;
		if(llvm_func->getReturnType() == mBuilder.getVoidTy()) {
			ret = mBuilder.CreateRetVoid();
		} else {
			llvm::Value* val = nullptr;
			if(llvm_func->getReturnType()->getTypeID() == Type::PointerTyID) {
				// Cast it to pointer type
				const string& str =  expected->toString();
				if(str.find('.') != string::npos)
					throw Exception("Floating point values are not valid for returning as pointer type!");
				unsigned bitwidth = llvm_func->getReturnType()->getPointerElementType()->getIntegerBitWidth();
				APInt int_value =  getAPIntTruncating(bitwidth, str, 10);
				ConstantInt* int_constant = ConstantInt::get
						(mModule->getContext(), int_value);
				AllocaInst* ptr_val = mBuilder.CreateAlloca(llvm_func->getReturnType()->getPointerElementType());
				AllocaInst* ptr = mBuilder.CreateAlloca(llvm_func->getReturnType());

				StoreInst* stor = mBuilder.CreateStore(int_constant, ptr_val);
				LoadInst* load = mBuilder.CreateLoad(ptr_val, false);
				SExtInst* sext = cast<SExtInst>(mBuilder.CreateSExt(load, mBuilder.getInt64Ty()));
				CastInst* int_to_ptr =
						cast<CastInst>(
						mBuilder.CreateIntToPtr
								(load, llvm_func->getReturnType()));
				StoreInst* fin = mBuilder.CreateStore(int_to_ptr, ptr);
				LoadInst* load_1 = mBuilder.CreateLoad(ptr);
				MB->getInstList().push_back(ptr_val);
				MB->getInstList().push_back(ptr);
				MB->getInstList().push_back(stor);
				MB->getInstList().push_back(load);
				MB->getInstList().push_back(sext);
				MB->getInstList().push_back(int_to_ptr);
				MB->getInstList().push_back(fin);
				MB->getInstList().push_back(load_1);
				ret = mBuilder.CreateRet(load_1);
			} else {
				val = createValue(llvm_func->getReturnType(), expected->toString());
				ret = mBuilder.CreateRet(val);
			}
		}
		MB->getInstList().push_back(ret);
		///////////////////////////////////////////////////////

		const string& fp_name = "gvar_fp_"+func_name;
		const string& wrapper_name = "wrapper_"+func_name;
		GlobalVariable* g_fp = mModule->getGlobalVariable(fp_name);
		Function* WrapperF = mModule->getFunction(wrapper_name);
		if(!WrapperF){
			///////////////////////////////////////////////////////
			// Create a function pointer which we will change at will to point
			// to either the mockup function or the original function
			PointerType* PointerTy_0 = PointerType::get(FT, 0);

			if(!g_fp) {
				g_fp = new GlobalVariable(/*Module=*/*mModule,
										 /*Type=*/PointerTy_0,
										 /*isConstant=*/false,
										 /*Linkage=*/GlobalValue::ExternalLinkage,
										 /*Initializer=*/0, // has initializer, specified below
										 /*Name=*/fp_name);
				g_fp->setAlignment(8);
			}
			//////////////////////////////////////////////////////
			// Create the wrapper function where we call the function pointer
			FunctionType* WrapperFType = FunctionType::get(llvm_func->getReturnType(),ArrayRef<Type*>(params),false);
			WrapperF = cast<Function>(mModule->getOrInsertFunction(wrapper_name, WrapperFType, llvm_func->getAttributes()));
			std::vector<Value*> args;
			llvm::Function::arg_iterator arg = nullptr;
			for(arg = WrapperF->arg_begin(); arg != WrapperF->arg_end(); ++arg)
					args.push_back(arg);

			LoadInst* load = mBuilder.CreateLoad(g_fp);
			CallInst* call = mBuilder.CreateCall(load, args);
			call->setCallingConv(llvm_func->getCallingConv());
			call->setTailCall(false);

			// create unique name
			BasicBlock* WrapperBlock = BasicBlock::Create(mModule->getContext(),"block_"+mockup_name,WrapperF);
			WrapperBlock->getInstList().push_back(load);
			WrapperBlock->getInstList().push_back(call);
			WrapperBlock->getInstList().push_back(mBuilder.CreateRet(call));

			//////////////////////////////////////////////////////
			// This is an important step: Replace all uses of the original function
			// with our wrapper function which calls the function pointer
			llvm_func->replaceAllUsesWith(WrapperF);
			// Make the function pointer point to the original function so we
			// don't affect the tests that do not use a mockup
			g_fp->setInitializer(llvm_func);
		}
		assert(g_fp && WrapperF && "Either function pointer or wrapper function have not been created");
		mFunctionsWrapped[func_name] = WrapperF;

		//////////////////////////////////////////////////////
		// Create two functions that change the function pointer with an LLVM
		// function. We could retrieve the function pointer address, but doing
		// so requires that we know the size of it, and as far as I know the
		// size of a function pointer is platform dependent. I may be wrong,
		// but just in case just create these two LLVM functions because I have
		// to meet the deadline! :)
		std::vector<Type*> void_args;
		FunctionType* VoidFunction = FunctionType::get(
								/*Result=*/Type::getVoidTy(mModule->getContext()),
								/*Params=*/void_args,
								/*isVarArg=*/false);
		//////////////////////////////////////////////////////
		// LLVM Function which changes the function pointer to point to the
		// mockup function
		Function* change_to_mockup =
				cast<Function>
				(mModule->getOrInsertFunction
						("from_"+func_name+"_to_"+mockup_name, VoidFunction));
		change_to_mockup->setLinkage(GlobalValue::ExternalLinkage);
		change_to_mockup->setCallingConv(CallingConv::C);
		BasicBlock* B_2 = BasicBlock::Create
				(mModule->getContext(),"change_to_"+mockup_name+"_block",
														change_to_mockup);
		StoreInst* store_2 =
				mBuilder.CreateStore(mockup_function, g_fp);
		ReturnInst* return_2 = mBuilder.CreateRetVoid();
		B_2->getInstList().push_back(store_2);
		B_2->getInstList().push_back(return_2);

		MF->setMockupFunction(change_to_mockup);

		//////////////////////////////////////////////////////
		// LLVM Function which changes the function pointer to point to the
		// original function
		Function* change_to_original =
				cast<Function>
				(mModule->getOrInsertFunction
						("from_"+mockup_name+"_to_"+func_name, VoidFunction));
		change_to_original->setLinkage(GlobalValue::ExternalLinkage);
		change_to_original->setCallingConv(CallingConv::C);
		BasicBlock* B_3 = BasicBlock::Create(mModule->getContext(),"from_"+mockup_name+"_to_"+func_name+"_block",change_to_original);
		StoreInst* store_3 = mBuilder.CreateStore(llvm_func, g_fp);
		ReturnInst* return_3 = mBuilder.CreateRetVoid();
		B_3->getInstList().push_back(store_3);
		B_3->getInstList().push_back(return_3);

		MF->setOriginalFunction(change_to_original);
		//////////////////////////////////////////////////////
	}
}

/**
 * Creates LLVM IR code for a single global variable assignment.
 *
 */
void TestGeneratorVisitor::VisitVariableAssignment(VariableAssignment *VA)
{
	string variable_name = VA->getIdentifier()->toString();
	GlobalVariable* global_variable = mModule->getGlobalVariable(variable_name);
	assert(global_variable && "Variable not found!");
	TokenType tokenType = VA->getTokenType();
	string real_value;

	if(VA->getConstant()) {
		real_value = VA->getConstant()->toString();
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
			AllocaInst* alloc = bufferAllocInitialization(global_variable->getType()->getElementType(), ba, mInstructions);
			StoreInst *store2 = mBuilder.CreateStore(alloc, global_variable); // Store an already allocated variable address to our pointer
			// alloc was already pushed inside bufferAllocInitialization
			mInstructions.push_back(store2);
		}
		break;
		default:
			assert(false && "Unhandled case! FIX ME");
			break;
	}
}

void TestGeneratorVisitor::VisitTestDefinitionFirst(TestDefinition *TD)
{
    mCurrentFud = TD->getTestFunction()->getFunctionCall()->getIdentifier()->toString();
}

void TestGeneratorVisitor::VisitTestSetup(TestSetup *TS)
{
    // Any variable assignment done in the current test pass it to the vector for
    // the current test.
    for(auto& b : mBackupTemp)
        mBackupTest.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}
void TestGeneratorVisitor::VisitTestFunction(TestFunction *)
{
	mFUDReturnValue = mReturnValue;
}
void TestGeneratorVisitor::VisitTestTeardown(TestTeardown *TT)
{
    // Any variable assignment done in the current test pass it to the vector for
    // the current test
    for(auto& b : mBackupTemp)
        mBackupTest.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}

void TestGeneratorVisitor::VisitTestDefinition(TestDefinition *TD)
{
	// Create the global variable that will hold the bool value for the comparison
	/// between the actual result and the expected result.
	GlobalVariable* result = new GlobalVariable(*mModule,
					mBuilder.getInt8Ty(),
					false,
					GlobalValue::LinkageTypes::ExternalLinkage,
					mBuilder.getInt8(1), // test passes by default
					"result_test_"+mCurrentFud); // Providing an initializer 'DEFINES' the variable
	// If zext == nullptr no expected expression was provided and we assume
	// the test passed.
	if(mTestResult) {
		// if mTestResult != nullptr means that there was an expected expression and
		// we should store the bool result in the global variable we just created.
		StoreInst* st = mBuilder.CreateStore(mTestResult, result);
		mInstructions.push_back(st);
		mTestResult = nullptr;
	}
	TD->setGlobalVariable(result);

    if(mBackupTest.size() )
        restoreGlobalVariables(mBackupTest);

    string func_name = "test_"+TD->getTestFunction()->getFunctionCall()->getIdentifier()->toString();
    Function *testFunction = generateFunction(func_name, true, mInstructions);
	TD->setLLVMFunction(testFunction);
    // The warnings may include test-setup, test-function, or test-teardown
    TD->setWarnings(mWarnings);

    mWarnings.clear();
}

void TestGeneratorVisitor::VisitGroupSetup(GlobalSetup *GS)
{
    string func_name = "group_setup_"+GS->getGroupName();
    Function *testFunction = generateFunction(func_name, false, mInstructions);
    GS->setLLVMFunction(testFunction);

    // Any variable assignment done in the current group pass it to the vector for
    // the current group.
    for(auto& b : mBackupTemp)
        mBackupGroup.push_back(b);

    while(mBackupTemp.size()) mBackupTemp.pop_back();
}

void TestGeneratorVisitor::VisitGroupTeardown(GlobalTeardown *GT)
{
    string func_name = "group_teardown_"+GT->getGroupName();
    Function *testFunction = generateFunction(func_name, false, mInstructions);
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
        Function *cleanupFUnction = generateFunction(func_name, false, mInstructions);
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

		if (IntegerType * intType = dyn_cast<IntegerType>(type)) {
			unsigned bitwidth = intType->getBitWidth();
			int radix = 10;
			size_t pos = value.find('x');
			if(pos != string::npos) {
				radix = 16;
				value = value.substr(pos+1,value.size()-pos+1);
			}
			APInt int_value = getAPIntTruncating(bitwidth, value, radix);
			return cast<Value>(mBuilder.getInt(int_value));
		}
	}
		break;
	// Even though this might look a repeated case like DoubleTyID, I will just
	// leave it here to remember that I need to do more research on how LLVM API
	// handles floating point values
	case Type::TypeID::FloatTyID:
	{
		string value =  real_value;
		size_t pos = value.find('x');
		if(pos != string::npos)
			value += "fp0";
		llvm::Constant* c = ConstantFP::get(type, value);
		return c;
	}
		break;
	case Type::TypeID::DoubleTyID:
	{
		string value =  real_value;
		size_t pos = value.find('x');
		if(pos != string::npos)
			value += "fp0";
		llvm::Constant* c = ConstantFP::get(type, real_value);
		return c;
	}
		break;
	case Type::TypeID::PointerTyID:
		mWarnings.push_back(
				Exception("Trying to create pointer from value "+real_value+
				", I will return a pointer to NULL instead.",
				"", Exception::WARNING));
		if (PointerType * ptrType = dyn_cast<PointerType>(type)) {
			return ConstantPointerNull::get(ptrType);
		}
		break;
	case Type::TypeID::VoidTyID:
		assert(false && "Cannot create a void value!");
		break;
	default:
		cerr << "Could not create value for typeID: " << typeID << endl;
		return nullptr;
	}
	return nullptr;
}

llvm::APInt TestGeneratorVisitor::getAPIntTruncating(unsigned bitwidth, const string& value, unsigned radix)
{
// Watch for the radix, right now we use radix 10 only
	unsigned bits_needed = APInt::getBitsNeeded(StringRef(value), radix);
	APInt int_value;
	if(bits_needed > bitwidth) {
		APInt i(bits_needed, value, radix);
		int_value = i.trunc(bitwidth);
		stringstream ss;
		ss <<"Truncating integer value "<<value<<"(decimal) from "
		   <<bits_needed<<" bits to "<<bitwidth<<" bits";
		mWarnings.push_back(Exception(ss.str(),"",Exception::WARNING));
	} else {
		int_value = APInt(bitwidth, value, radix);
	}
	return int_value;
}

llvm::Function* TestGeneratorVisitor::generateFunction(
		const string& name, bool use_mFUDReturnValue,
		vector<llvm::Instruction*>& instructions)
{
    string unique_name = getUniqueTestName(name);
	llvm::Type* return_type = nullptr;
	if(mTestFunctionCall && use_mFUDReturnValue)
		return_type = mTestFunctionCall->getCalledFunction()->getReturnType();
	else
		return_type = mBuilder.getInt32Ty();
	Function *function = cast<Function> (mModule->getOrInsertFunction(
			unique_name,
			return_type,
			nullptr));
	BasicBlock *BB = BasicBlock::Create(mModule->getContext(),
			"block_" + unique_name, function);

	ReturnInst *ret = nullptr;
	if(use_mFUDReturnValue && mFUDReturnValue) {
		ret = mBuilder.CreateRet(mFUDReturnValue);
	}
	else
            ret = mBuilder.CreateRet(mBuilder.getInt32(1));// the setup/teardown passed

	instructions.push_back(ret);

	mBuilder.SetInsertPoint(BB);
	for (auto*& inst : instructions)
		mBuilder.Insert(inst);

	instructions.clear();
	mBackup.clear();
	mBuilder.ClearInsertionPoint();
	mReturnValue = nullptr;
	mTestFunctionCall = nullptr;

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
				str_value = arg->getArgument().toString();
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

llvm::AllocaInst* TestGeneratorVisitor::bufferAllocInitialization(llvm::Type* ptrType, const tp::BufferAlloc *ba,
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

//////////////////////////////////////////////////////////////////////
// Replace all the DataPlaceholders to generate all the functions

void DataPlaceholderVisitor::VisitTestDefinition(TestDefinition* TD)
{
	TestData *d = TD->getTestData();
	TestFunction* TF = TD->getTestFunction();

	if (d == nullptr && TF->hasDataPlaceholders()) {
		string func = TF->getFunctionCall()->getFunctionCalledString();
		ExpectedResult* R = TF->getExpectedResult();
		if(R && R->isDataPlaceholder())
			func += " " + R->getComparisonOperator()->toString() + " @";
		throw Exception("Missing CSV file for this test: "+func+";\n"
				"\tAdd it with the statement: 'data { \"path-to-file.csv\"; }'");
	}

	if (d && TF->hasDataPlaceholders() == false)
		return; // Do not open file

	if (d && TF->hasDataPlaceholders()) {
		string path = d->getDataPath();
		CSVDriver csv(path);

		unsigned placeholder_count = TF->getFunctionCall()->getDataPlaceholdersPos().size();
		ExpectedResult* R = TF->getExpectedResult();
		if(R)
			placeholder_count += (R->isDataPlaceholder())?1:0;

		if(placeholder_count > csv.columnCount()) {
			string func = TF->getFunctionCall()->getFunctionCalledString();
			if(R && R->isDataPlaceholder())
				func += " " + R->getComparisonOperator()->toString() + " @";
			throw Exception("Not enough data in CVS file "+path+" for function "+func);
		}

		vector<TestDefinition*> to_be_added;
		for(unsigned i = 0; i < csv.rowCount(); ++i) {
			///////////////////////////////////////
			// Copy the test definition N times
			TestDefinition* copy = new TestDefinition(*TD);

			FunctionCall* FC = copy->getTestFunction()->getFunctionCall();
			vector<unsigned> dp_positions =  FC->getDataPlaceholdersPos();
			unsigned j = 0;
			for(unsigned pos : dp_positions)
				FC->replaceDataPlaceholder(pos, csv.getFunctionArgumentAt(i,j++));

			ExpectedResult* ER = copy->getTestFunction()->getExpectedResult();
			if(ER && ER->isDataPlaceholder())
				ER->replaceDataPlaceholder(csv.getExpectedConstantAt(i,j++));

			to_be_added.push_back(copy);
			//
			/////////////////////////////////////////
		}
		if(to_be_added.size()) {
			mReplacements[TD] = to_be_added;
			mTests.push(TD);
		}
	}
}

void DataPlaceholderVisitor::VisitTestGroupFirst(TestGroup*)
{
	mTests.push(nullptr);
}

void DataPlaceholderVisitor::VisitTestGroup(TestGroup* TG)
{
	vector<TestExpr*>& tests = TG->getTests();
	TestDefinition* td = nullptr;
	while((td = mTests.top()) != nullptr) {
		vector<TestExpr*>::iterator to_remove = find(tests.begin(),tests.end(), td);
		if(to_remove != tests.end()) { // Found
			delete *to_remove;
			*to_remove = nullptr;
			vector<TestDefinition*>& replacements = mReplacements[td];
			vector<TestExpr*>::iterator new_pos = tests.erase(to_remove);
			tests.insert(new_pos, replacements.begin(), replacements.end());
			mReplacements.erase(td);
			mTests.pop();
		}
	}
	assert(mTests.top() == nullptr && "Invalid DataPlaceholder replacement!");
	mTests.pop();
}
