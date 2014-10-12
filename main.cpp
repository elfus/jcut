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
#include "llvm/Support/Debug.h"
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
#include "clang/Frontend/CompilerInstance.h"

#include "TestParser.h"
#include "TestGeneratorVisitor.h"
#include "TestRunnerVisitor.h"
#include "TestLoggerVisitor.h"

#include "linenoise.h"

#include <vector>
#include <iostream>

using namespace std;

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory JcutOptions("JCUT Options");
// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

static cl::opt<string> TestFileOpt("t", cl::Optional,  cl::ValueRequired, cl::desc("Input test file"), cl::value_desc("filename"));
static cl::opt<bool> DumpOpt("dump", cl::init(false), cl::Optional, cl::desc("Dump generated LLVM IR code"), cl::value_desc("filename"));
static int TotalTestsFailed = 0;

class JCUTAction : public clang::EmitLLVMOnlyAction {
#undef DEBUG_TYPE
#define DEBUG_TYPE "JCUTAction"
public:
	/* These two static variables are used as workaround to communicate with
	 * the main execution flow. Keep in mind that a JCUTAction will be
	 * created for each source file, thus its lifetime is less than that of
	 * the interpreter itself. Another thing is that we cannot get a direct
	 * handle to the JCUTAction object when it's created with the
	 * newFrontendActionFactory<T>() method. That's why we use these two
	 * static variables.
	 */
	static bool mUseInterpreterInput;
	static string mInterpreterInput;
	JCUTAction() { }

	bool BeginInvocation(CompilerInstance& CI) {
		DEBUG(errs() << "'JCUTAction' Beginning invocation\n");
		return true;
	}

	bool BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
		DEBUG(errs() << "'JCUTAction' BeginSourceFileAction on "
				     << Filename.str() << "\n");
		/* This is an important step: The backend needs to know which C source
		 * file to use, otherwise it fail saying that clang needs 1 positional
		 * argument.
		 */
		CI.getCodeGenOpts().BackendOptions.push_back(Filename.str());
		return true;
	}

	void EndSourceFileAction() {
		DEBUG(errs() << "'JCUTAction' EndSourceFileAction\n");

		EmitLLVMOnlyAction::EndSourceFileAction();
		// The JIT Will take ownership of the Module!
		llvm::Module* module = takeModule();

		try {
			TestDriver driver;
			if(mUseInterpreterInput) {
				JCUTException::mExceptionSource = "jcut interpreter";
				driver.tokenize(mInterpreterInput.c_str());
			}
			else {
				JCUTException::mExceptionSource = TestFileOpt.getValue(); // quick workaround
				driver.tokenize(TestFileOpt.getValue());
			}
			unique_ptr<TestExpr> tests (driver.ParseTestExpr()); // Parse file and generate object structure tree

			DataPlaceholderVisitor dp;
			tests->accept(&dp); // Generate functions using data place holders.

			TestGeneratorVisitor visitor(module);
			tests->accept(&visitor); // Generate LLVM IR code

			std::string Error;
			TestRunnerVisitor runner(llvm::ExecutionEngine::createJIT(module, &Error),DumpOpt.getValue(),module);
			if (runner.isValidExecutionEngine() == false) {
				llvm::errs() << "unable to make execution engine: " << Error << "\n";
				return;
			}
			tests->accept(&runner);

			TestLoggerVisitor results_logger;
			OutputFixerVisitor fixer(results_logger);
			results_logger.setLogFormat(TestLoggerVisitor::LOG_ALL);
			// @note The following two calls have to happen in this exact
			// same order:
			//  1st OutputFixerVisitor so we can get the right widths of all the output
			//  2nd TestLoggerVisitor so we can print the test information
			tests->accept(&fixer);
			tests->accept(&results_logger);

			// this application exits with the number of tests failed.
			TotalTestsFailed += results_logger.getTestsFailed();
		} catch(const UnexpectedToken& e){
			errs() << e.what() << "\n";
		}
		catch (const JCUTException& e) {
			errs() << e.what() << "\n";
		}
	}
#undef DEBUG_TYPE
};

bool JCUTAction::mUseInterpreterInput = false;
string JCUTAction::mInterpreterInput = "";

int batchMode(CommonOptionsParser& OptionsParser) {
	// Use OptionsParser.getCompilations() and OptionsParser.getSourcePathList()
	// to retrieve CompilationDatabase and the list of input file paths.

	CompilationDatabase& CD = OptionsParser.getCompilations();
	// A clang tool can run over a number of sources in the same process...
	std::vector<std::string> Sources = OptionsParser.getSourcePathList();

	// We hand the CompilationDatabase we created and the sources to run over into
	// the tool constructor.
	ClangTool Tool(CD, Sources);

	FrontendActionFactory* syntax_action = newFrontendActionFactory<clang::SyntaxOnlyAction>();
	// The ClangTool needs a new FrontendAction for each translation unit we run
	// on.  Thus, it takes a FrontendActionFactory as parameter.  To create a
	// FrontendActionFactory from a given FrontendAction type, we call
	// newFrontendActionFactory<clang::SyntaxOnlyAction>().
	int failed = Tool.run(syntax_action);
	if(failed)
		return -1;

	FrontendActionFactory* jcut_action = newFrontendActionFactory<JCUTAction>();
	failed = Tool.run(jcut_action);

	return TotalTestsFailed;
}
//////////////////
// Interpreter specific functions
void completionCallBack(const char * line, linenoiseCompletions *lc) {

	if (line[0] == 'b') {
		linenoiseAddCompletion(lc,"before");
		linenoiseAddCompletion(lc,"before_all");
	}

	if (line[0] == 'a') {
		linenoiseAddCompletion(lc,"after");
		linenoiseAddCompletion(lc,"after_all");
	}

	if (line[0] == 'm') {
		linenoiseAddCompletion(lc,"mockup");
		linenoiseAddCompletion(lc,"mockup_all");
	}

	if (line[0] == 'd')
		linenoiseAddCompletion(lc,"data");

	if (line[0] == 'g')
		linenoiseAddCompletion(lc,"group");

}

int interpreterMode(CommonOptionsParser& OptionsParser) {
	cout << "Interpreter mode!" << endl;
	char *c_line = nullptr;
	string history_name = "jcut-history.txt";
	linenoiseHistoryLoad(history_name.c_str()); /* Load the history at startup */
	string line;
	const string prompt_input = "jcut $> ";
	const string prompt_more = "jcut ?> ";
	string prompt = prompt_input;
	linenoiseSetCompletionCallback(completionCallBack);
	JCUTAction::mUseInterpreterInput = true;

	while((c_line = linenoise(prompt.c_str())) != nullptr) {
		line = string(c_line);
		// Process interpreter specific commands first
		if(line == "exit") {
			free(c_line);
			break;
		}
		if (line == "/historylen") {
			/* The "/historylen" command will change the history len. */
			int len = atoi(c_line+11);
			linenoiseHistorySetMaxLen(len);
		} else if (line.size() && line[0] == '/') {
			cout << "Unreconized command: "<< line << endl;;
		}

		if(linenoiseCtrlJPressed()) {
			prompt = prompt_more;
			if(line.empty()) {
				prompt = prompt_input;
				linenoiseCtrlJClear();
			}
		}

		JCUTAction::mInterpreterInput += line;
		if(prompt == prompt_input) {
			batchMode(OptionsParser);
			JCUTAction::mInterpreterInput.clear();
		}

		/* Do something with the string. */

		// Save it to the history
		if(line.size()) {
			linenoiseHistoryAdd(line.c_str());
			linenoiseHistorySave(history_name.c_str());
		}

		free(c_line);
		// Repeat
	}
	return 0;
}

int main(int argc, const char **argv, char * const *envp)
{
	TestFileOpt.setCategory(JcutOptions);
	DumpOpt.setCategory(JcutOptions);

	// CommonOptionsParser constructor will parse arguments and create a
	// CompilationDatabase.  In case of error it will terminate the program.
	CommonOptionsParser OptionsParser(argc, argv);

	// Initialize the JIT Engine only once
	llvm::InitializeNativeTarget();

	if(TestFileOpt.size())
		return batchMode(OptionsParser);
	else
		return interpreterMode(OptionsParser);
}
