//===-- examples/clang-interpreter/main.cpp - Clang C Interpreter Example -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <utility>
using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace llvm;

static void createMockFunction(llvm::Module *Mod, llvm::Function *Function);
static llvm::Function* createWrapperFunction(llvm::Module *Mod,
		llvm::Function *Wrapped, const vector<string>& Args);

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).

std::string GetExecutablePath(const char *Argv0)
{
	// This just needs to be some symbol in the binary; C++ doesn't
	// allow taking the address of ::main however.
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

/**
 * Represents a function to be unit tested.
 */
struct TestFunction {
	string FunctionName; //!< The name of the function as it's spelled in source code.
	vector<string> FunctionArguments; //!< Arguments to be passed to the function.
};


/**
 * Reads the arguments from the command to extract the function we want to test.
 *
 * @note Args was previously parsed by clang parsing system.
 *
 * @param Args Command line arguments.
 * @return A TestFunction
 */
TestFunction extractTestFunction(SmallVector<const char *, 16> & Args)
{
	TestFunction testFunction;
	unsigned delete_count = 1; // remove the flag --test

	for (unsigned i = 0; i < Args.size(); i++) {
		if (string(Args[i]) == "--test") {
			++delete_count;
			testFunction.FunctionName = string(Args[++i]);
			while (++i < Args.size()) {
				testFunction.FunctionArguments.push_back(string(Args[i]));
				++delete_count;
			}
		}
	}

	if (delete_count > 1)
		while (delete_count--)
			Args.pop_back();
	
	return std::move(testFunction);
}

/**
 * Finds all the functions lacking an implementation in the given function.
 * The search for declaration is made recursively, meaning that that all the
 * functions called by the given llvm::Function f will be evaluated.
 *
 * @param f Function to check if it's a declaration.
 * @return
 */
static vector<llvm::Function*> FindFunctionDeclaration(llvm::Function *f)
{
	static vector<llvm::Function*> declarations;

	if (f->isDeclaration())
		declarations.push_back(f);
	else {
		for (Function::iterator i = f->begin(); i != f->end(); i++) {
			for (BasicBlock::iterator j = i->begin(); j != i->end(); j++) {
				if (CallInst * call = dyn_cast<CallInst>(&*j)) {
					Function *tmp = call->getCalledFunction();
					FindFunctionDeclaration(tmp);
				}
			}
		}
	}
	
	return declarations;
}

static int Execute(llvm::Module *Mod, char * const *envp, const TestFunction& jitFunc)
{
	llvm::InitializeNativeTarget();

	std::string Error;
	OwningPtr<llvm::ExecutionEngine> EE(
			llvm::ExecutionEngine::createJIT(Mod, &Error));
	if (!EE) {
		llvm::errs() << "unable to make execution engine: " << Error << "\n";
		return 255;
	}

	string functionName = jitFunc.FunctionName.empty() ? "main" : jitFunc.FunctionName;

	llvm::Function *EntryFn = Mod->getFunction(functionName);
	if (!EntryFn) {
		llvm::errs() << functionName << " function not found in module.\n";
		return 255;
	}

	vector<llvm::Function*> decl = FindFunctionDeclaration(EntryFn);

	// Do we want to provide a mock function?
	// IF yes provide a mockup function
	for (auto F : decl) {
		// For the moment just create a mockup function for the mult function
		// which will add rather than multiply
		if (F->getName().str() == "mult") {
			createMockFunction(Mod, F);
		}
	}
	//IF NOT let the program finish with an error about an undefined function

	// Arguments from command line
	std::vector<llvm::GenericValue> Args;
	llvm::GenericValue v;
	for (auto arg : jitFunc.FunctionArguments) {
		v.IntVal = llvm::APInt(32, arg, 10);
		//Args.push_back(v);
	}

	Function *wrapper = createWrapperFunction(Mod, EntryFn, jitFunc.FunctionArguments);

	//Args.push_back(Mod->getModuleIdentifier());
	return *(EE->runFunction(wrapper, Args).IntVal.getRawData());
}

int main(int argc, const char **argv, char * const *envp)
{
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	std::string Path = GetExecutablePath(argv[0]);
	IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
	TextDiagnosticPrinter *DiagClient =
			new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

	IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
	DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);
	Driver TheDriver(Path, llvm::sys::getProcessTriple(), "a.out", Diags);
	TheDriver.setTitle("clang interpreter");

	// FIXME: This is a hack to try to force the driver to do something we can
	// recognize. We need to extend the driver library to support this use model
	// (basically, exactly one input, and the operation mode is hard wired).
	SmallVector<const char *, 16> Args(argv, argv + argc);
	TestFunction testFunction = extractTestFunction(Args);
	Args.push_back("-fsyntax-only");
	OwningPtr<Compilation> C(TheDriver.BuildCompilation(Args));
	if (!C)
		return 0;

	// FIXME: This is copied from ASTUnit.cpp; simplify and eliminate.

	// We expect to get back exactly one command job, if we didn't something
	// failed. Extract that job from the compilation.
	const driver::JobList &Jobs = C->getJobs();
	if (Jobs.size() != 1 || !isa<driver::Command>(*Jobs.begin())) {
		SmallString<256> Msg;
		llvm::raw_svector_ostream OS(Msg);
		Jobs.Print(OS, "; ", true);
		Diags.Report(diag::err_fe_expected_compiler_job) << OS.str();
		return 1;
	}

	const driver::Command *Cmd = cast<driver::Command>(*Jobs.begin());
	if (llvm::StringRef(Cmd->getCreator().getName()) != "clang") {
		Diags.Report(diag::err_fe_expected_clang_command);
		return 1;
	}

	// Initialize a compiler invocation object from the clang (-cc1) arguments.
	const driver::ArgStringList &CCArgs = Cmd->getArguments();
	OwningPtr<CompilerInvocation> CI(new CompilerInvocation);
	CompilerInvocation::CreateFromArgs(*CI,
			const_cast<const char **> (CCArgs.data()),
			const_cast<const char **> (CCArgs.data()) +
			CCArgs.size(),
			Diags);

	// Show the invocation, with -v.
	if (CI->getHeaderSearchOpts().Verbose) {
		llvm::errs() << "clang invocation:\n";
		Jobs.Print(llvm::errs(), "\n", true);
		llvm::errs() << "\n";
	}

	// FIXME: This is copied from cc1_main.cpp; simplify and eliminate.

	// Create a compiler instance to handle the actual work.
	CompilerInstance Clang;
	Clang.setInvocation(CI.take());

	// Create the compilers actual diagnostics engine.
	Clang.createDiagnostics();
	if (!Clang.hasDiagnostics())
		return 1;

	// Infer the builtin include path if unspecified.
	if (Clang.getHeaderSearchOpts().UseBuiltinIncludes &&
			Clang.getHeaderSearchOpts().ResourceDir.empty())
		Clang.getHeaderSearchOpts().ResourceDir =
			CompilerInvocation::GetResourcesPath(argv[0], MainAddr);

	// Create and execute the frontend to generate an LLVM bitcode module.
	OwningPtr<CodeGenAction> Act(new EmitLLVMOnlyAction());
	if (!Clang.ExecuteAction(*Act))
		return 1;

	int Res = 255;
	if (llvm::Module * Module = Act->takeModule())
		Res = Execute(Module, envp, testFunction);

	// Shutdown.

	llvm::llvm_shutdown();

	cout << "Function call Result: " << Res << endl;

	return Res;
}

void createMockFunction(llvm::Module *Mod, llvm::Function *Function)
{
	// Create the add1 function entry and insert this entry into module M.  The
	// function will have a return type of "int" and take an argument of "int".
	// The '0' terminates the list of argument types.
	//	Function *Add1F =
	//			cast<Function>(M->getOrInsertFunction("add1", Type::getInt32Ty(Context),
	//			Type::getInt32Ty(Context),
	//			(Type *) 0
	cout << "Creating mock function...";

	// Add a basic block to the function. As before, it automatically inserts
	// because of the last argument.
	BasicBlock *BB = BasicBlock::Create(Mod->getContext(), "EntryBlock", Function);

	// Create a basic block builder with default parameters.  The builder will
	// automatically append instructions to the basic block `BB'.
	IRBuilder<> builder(BB);

	// Get pointers to the constant `1'.
	//	Value *One = builder.getInt32(1);

	// Get pointers to the integer argument of the add1 function...
	assert(Function->arg_begin() != Function->arg_end()); // Make sure there's an arg
	Argument *ArgX = Function->arg_begin(); // Get the arg
	ArgX->setName("AnArg"); // Give it a nice symbolic name for fun
	Argument *Arg2 = ++(Function->arg_begin());

	// Create the add instruction, inserting it into the end of BB.
	Value *Add = builder.CreateAdd(ArgX, Arg2);
	
	// Create the return instruction and add it to the basic block
	builder.CreateRet(Add);

	cout << "creation completed." << endl;
	outs() << *Function << "\n";
	cout << "Number of arguments: " << Function->arg_size() << endl;
	// Now, function add1 is ready.
}

llvm::Function* createWrapperFunction(llvm::Module *Mod, llvm::Function *Wrapped,
		const vector<string>& Args)
{
	Function *Wrapper = cast<Function> (Mod->getOrInsertFunction("wrapperFunc",
			Type::getVoidTy(Mod->getContext()),
			(Type*) 0));

	// Add a basic block to the function. As before, it automatically inserts
	// because of the last argument.
	BasicBlock *BB = BasicBlock::Create(Mod->getContext(), "wrapperBlock", Wrapper);

	IRBuilder<> builder(BB);

	vector<llvm::Value*> args;

	Argument &arg = *(Wrapped->arg_begin());
	// Allocate first argument
	AllocaInst *alloc = builder.CreateAlloca(arg.getType(), 0, "Allocation1");
	alloc->setAlignment(4);
	StoreInst *store = builder.CreateStore(builder.getInt32(2), alloc);
	LoadInst *load1 = builder.CreateLoad(alloc, "value1");
	args.push_back(load1);

	// Allocate second argument
	AllocaInst *alloc2 = builder.CreateAlloca(arg.getType(), 0, "Allocation2");
	alloc2->setAlignment(4);
	StoreInst *store2 = builder.CreateStore(builder.getInt32(6), alloc2);
	LoadInst *load2 = builder.CreateLoad(alloc2, "value2");
	args.push_back(load2);

	AllocaInst *alloc3 = builder.CreateAlloca(Type::getInt32PtrTy(Mod->getContext()), 0, "Allocation3");
	alloc3->setAlignment(8);
	StoreInst *store3 = builder.CreateStore(alloc, alloc3);

	CallInst * wrappedCall = builder.CreateCall(Wrapped, args);

	builder.CreateRet(wrappedCall);

	outs() << "Wrapper Function\n";
	Wrapper->dump();

	return Wrapper;
}