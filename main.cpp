//===-- jit-testing/main.cpp ------------------------------------*- C++ -*-===//
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
#include "TestParser.h"
#include "TestGeneratorVisitor.h"
#include "TestRunnerVisitor.h"
#include "TestLoggerVisitor.h"
#include <iostream>
#include <utility>
using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace llvm;

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
 * Reads the arguments from the command to extract the function we want to test.
 *
 * @note Args was previously parsed by clang parsing system.
 *
 * @param Args Command line arguments.
 * @return A TestFunction
 */
string extractTestFile(SmallVector<const char *, 16> & Args)
{
	string fileName;
	unsigned delete_count = 1; // remove the flag --test-file
	auto it = Args.begin();

	for (; it != Args.end(); it++) {
		if (string(*it) == "-t") {
			++delete_count;
			auto it_2 = it + 1;
			fileName = string(*it_2);
			Args.erase(it, it+2);// 1 past the end so we can erase the file name
			break;
		}
	}

	return fileName;
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
    if (argc < 4 && string(argv[1]).compare("--help") != 0 ) {
        cerr << "Wrong command line." << endl;
        cerr << "Usage:\n\t"<<argv[0]<< " <c files> -t <test-file> [--dump]"<<endl;
		cerr << argv[1] << endl;
        return -1;
    }
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
	string file_name = extractTestFile(Args);
	bool dump_functions = extractDumpFlag(Args);
	bool recover_on_parse_error = extractHaltFlag(Args);
	Args.push_back("-fsyntax-only");
	Args.push_back("-I include");// This is for windows only.
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
	if (llvm::Module * Module = Act->takeModule()) {
		if (file_name.empty() == false) {
			try {
				Exception::mCurrentFile = file_name; // quick workaround
				TestDriver driver(file_name, recover_on_parse_error);
				TestExpr *tests = driver.ParseTestExpr(); // Parse file
				TestGeneratorVisitor visitor(Module);
				tests->accept(&visitor); // Generate object structure tree

				// Initialize the JIT Engine only once
				llvm::InitializeNativeTarget();
				std::string Error;
				TestRunnerVisitor runner(llvm::ExecutionEngine::createJIT(Module, &Error),dump_functions,Module);
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
				Res = results_logger.getTestsFailed();
				delete tests;
			} catch (const Exception& e) {
				errs() << e.what() << "\n";
			}
		} else {
			assert(false && "Option not implemented.");
		}
	}
	// Shutdown.

	llvm::llvm_shutdown();

	if(TestExpr::leaks)
		cerr << TestExpr::leaks << " memory leaks in TestExpr!" << endl;

	return Res;
}
