//===-- jcut/main.cpp -------------------------------------------*- C++ -*-===//
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
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "clang/Driver/Compilation.h"
#include "clang/Driver/Job.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "TestParser.h"
#include "TestGeneratorVisitor.h"
#include "TestRunnerVisitor.h"
#include "TestLoggerVisitor.h"

#include <vector>
#include <iostream>

using namespace std;

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\njit-testing OPTIONS:\n");
static cl::opt<string> TestFileOpt("t", cl::Required, cl::desc("Input test file"), cl::value_desc("filename"));

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

static void removeJitTestingOptions(SmallVector<const char *, 16>& args) {
	SmallVector<const char *, 16>::iterator it = nullptr;
	for(it = args.begin(); it != args.end(); ++it) {
		if(string(*it).find("-t") != string::npos)
			args.erase(it);
			break;
	}
}

bool extractDumpFlag(SmallVector<const char *, 16> & Args)
{
	for (auto it = Args.begin(); it != Args.end(); it++) {
		if (string(*it) == "--dump") {
			Args.erase(it);
			return true;
		}
	}
	return false;
}

bool extractHaltFlag(SmallVector<const char *, 16> & Args)
{
	for (auto it = Args.begin(); it != Args.end(); it++) {
		// recover from parse errors
		if (string(*it) == "--recover") {
			Args.erase(it);
			return true;
		}
	}
	return false;
}

int main(int argc, const char **argv, char * const *envp)
{
	
    // CommonOptionsParser constructor will parse arguments and create a
	// CompilationDatabase.  In case of error it will terminate the program.
	CommonOptionsParser OptionsParser(argc, argv);
	// Use OptionsParser.getCompilations() and OptionsParser.getSourcePathList()
	// to retrieve CompilationDatabase and the list of input file paths.

	CompilationDatabase& CD = OptionsParser.getCompilations();
	// A clang tool can run over a number of sources in the same process...
	std::vector<std::string> Sources = OptionsParser.getSourcePathList();

	// We hand the CompilationDatabase we created and the sources to run over into
	// the tool constructor.
	ClangTool Tool(CD, Sources);

	FrontendActionFactory* action = newFrontendActionFactory<clang::SyntaxOnlyAction>();
	// The ClangTool needs a new FrontendAction for each translation unit we run
	// on.  Thus, it takes a FrontendActionFactory as parameter.  To create a
	// FrontendActionFactory from a given FrontendAction type, we call
	// newFrontendActionFactory<clang::SyntaxOnlyAction>().
	int failed = Tool.run(action);
	if(failed)
		return failed;
	
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	std::string Path = GetExecutablePath(argv[0]);
	IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
	TextDiagnosticPrinter *DiagClient =
			new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

	IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
	DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);
	driver::Driver TheDriver(Path, llvm::sys::getProcessTriple(), "a.out", Diags);
	TheDriver.setTitle("jit-testing");

	SmallVector<const char *, 16> Args(argv, argv + argc);

	bool dump_functions = extractDumpFlag(Args);
	bool recover_on_parse_error = extractHaltFlag(Args);
	removeJitTestingOptions(Args);
	Args.push_back("-I include");// This is for windows only.
	Args.push_back("-fsyntax-only"); // we don't want to link
	OwningPtr<clang::driver::Compilation> C(TheDriver.BuildCompilation(Args));
	if (!C)
		return -1;

	const clang::driver::JobList& Jobs = C->getJobs();

	if(Jobs.size() != Sources.size()) {
		SmallString<256> Msg;
		llvm::raw_svector_ostream OS(Msg);
		Jobs.Print(OS, "; ", true);
		Diags.Report(diag::err_fe_expected_compiler_job) << OS.str();
		return -1;
	}

	//////////////////////////////////////////
	// Generate LLVM IR Code for every job (source file)
	//
	vector<llvm::Module*> modules;
	//////////////////////////
	// These three vectors are used to keep the CompilerInvocation,
	// CompilerInstance and CodeGenAction objects alive so we can manipulate
	// all the llvm::Module later on.
	vector<OwningPtr<CompilerInvocation>> invocations;
	vector<OwningPtr<CompilerInstance>> compilers;
	vector<OwningPtr<CodeGenAction>> actions;
	//////////////////////////
	for(driver::Job* job : Jobs ) {
		// All the jobs (source files have the same command arguments)
		const llvm::opt::ArgStringList& CCArgs = cast<driver::Command>
												  (job)->getArguments();
		OwningPtr<CompilerInvocation> CI(new CompilerInvocation);
		CompilerInvocation::CreateFromArgs(*CI,
				const_cast<const char **> (CCArgs.data()),
				const_cast<const char **> (CCArgs.data()) +
				CCArgs.size(),
				Diags);

		OwningPtr<CompilerInstance> Clang(new CompilerInstance());
		Clang->setInvocation(CI.take());

		Clang->createDiagnostics();
		if(!Clang->hasDiagnostics())
			return -1;

		// Infer the builtin include path if unspecified.
		if (Clang->getHeaderSearchOpts().UseBuiltinIncludes &&
				Clang->getHeaderSearchOpts().ResourceDir.empty())
			Clang->getHeaderSearchOpts().ResourceDir =
				CompilerInvocation::GetResourcesPath(argv[0], MainAddr);

		// Create and execute the frontend to generate an LLVM bitcode module.
		OwningPtr<CodeGenAction> Act(new EmitLLVMOnlyAction());
		if (!Clang->ExecuteAction(*Act))
			return 1;

		if(llvm::Module* m = Act->takeModule())
			modules.push_back(m);

		// We need the CompilerInvocation and CompilerInstance to live
		// so the LLVM Modules are valid after exiting this for loop.
		invocations.push_back(OwningPtr<CompilerInvocation>(CI.take()));
		compilers.push_back(OwningPtr<CompilerInstance>(Clang.take()));
		actions.push_back(OwningPtr<CodeGenAction>(Act.take()));
	}

	for(llvm::Module* module : modules) {
		// add my code here
		try {
			Exception::mCurrentFile = TestFileOpt.getValue(); // quick workaround
			TestDriver driver(TestFileOpt.getValue(), recover_on_parse_error);
			TestExpr *tests = driver.ParseTestExpr(); // Parse file
			TestGeneratorVisitor visitor(module);
			tests->accept(&visitor); // Generate object structure tree

			// Initialize the JIT Engine only once
			llvm::InitializeNativeTarget();
			std::string Error;
			TestRunnerVisitor runner(llvm::ExecutionEngine::createJIT(module, &Error),dump_functions,module);
			if (runner.isValidExecutionEngine() == false) {
				llvm::errs() << "unable to make execution engine: " << Error << "\n";
				return 255;
			}
			tests->accept(&runner);

			TestLoggerVisitor results_logger;
			OutputFixerVisitor fixer(results_logger);
			results_logger.setLogFormat(TestLoggerVisitor::LOG_ALL);
//				| TestLoggerVisitor::LOG_TEST_SETUP| TestLoggerVisitor::LOG_TEST_TEARDOWN
//				| TestLoggerVisitor::LOG_TEST_CLEANUP | TestLoggerVisitor::LOG_GROUP_CLEANUP
//				| TestLoggerVisitor::LOG_GROUP_SETUP | TestLoggerVisitor::LOG_GROUP_TEARDOWN );
			// @note The following two calls have to happen in this exact
			// same order:
			//  1st OutputFixerVisitor so we can get the right widths of all the output
			//  2nd TestLoggerVisitor so we can print the test information
			tests->accept(&fixer);
			tests->accept(&results_logger);

			// this application exits with the number of tests failed.
			failed += results_logger.getTestsFailed();
			delete tests;
		} catch (const Exception& e) {
			errs() << e.what() << "\n";
		}
		break; //@todo Implement multiple C files
	}

	cout << "Number of jobs: " << Jobs.size() << endl;
	
	for(auto& file : Sources) {
		cout << "Source file: " << file << endl;
	}

	return failed;
}
