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

// For llvm::InitializeNativeTarget();
#include "llvm/Support/TargetSelect.h"

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "JCUTAction.h"
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

cl::opt<string> TestFileOpt("t", cl::Optional,  cl::ValueRequired, cl::desc("Input test file"), cl::value_desc("filename"));
cl::opt<bool> DumpOpt("dump", cl::init(false), cl::Optional, cl::desc("Dump generated LLVM IR code"), cl::value_desc("filename"));

static int batchMode(CommonOptionsParser& OptionsParser);
//////////////////
// Interpreter specific functions
static void processCommand(const string& cmd, CommonOptionsParser& OptionsParser);
static int interpreterMode(CommonOptionsParser& OptionsParser);
static void completionCallBack(const char * line, linenoiseCompletions *lc);

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

static int batchMode(CommonOptionsParser& OptionsParser) {
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

	FrontendActionFactory* jcut_action = newFrontendActionFactory<jcut::JCUTAction>();
	failed = Tool.run(jcut_action);

	return jcut::TotalTestsFailed;
}

static void completionCallBack(const char * line, linenoiseCompletions *lc)
{
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

static int interpreterMode(CommonOptionsParser& OptionsParser) {
	cout << "Interpreter mode!" << endl;
	char *c_line = nullptr;
	string history_name = "jcut-history.txt";
	linenoiseHistoryLoad(history_name.c_str()); /* Load the history at startup */
	string line;
	const string prompt_input = "jcut $> ";
	const string prompt_more = "jcut ?> ";
	string prompt = prompt_input;
	linenoiseSetCompletionCallback(completionCallBack);
	jcut::JCUTAction::mUseInterpreterInput = true;

	while((c_line = linenoise(prompt.c_str())) != nullptr) {
		line = string(c_line);
		if(line == "exit") {
			free(c_line);
			break;
		}
		processCommand(line,OptionsParser);


		if(linenoiseCtrlJPressed()) {
			prompt = prompt_more;
			if(line.empty()) {
				prompt = prompt_input;
				linenoiseCtrlJClear();
			}
		}

		jcut::JCUTAction::mInterpreterInput += line;
		if(prompt == prompt_input) {
			batchMode(OptionsParser);
			jcut::JCUTAction::mInterpreterInput.clear();
		}

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

static void processCommand(const string& cmd, CommonOptionsParser& OptionsParser)
{
	if (cmd.size() && cmd[0] == '/') {
		cout << "Unreconized command: "<< cmd << endl;;
	}
}

