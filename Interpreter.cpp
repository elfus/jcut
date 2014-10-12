/*
 * Interpreter.cpp
 *
 *  Created on: Oct 12, 2014
 *      Author: aortegag
 */

#include <iostream>

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Interpreter.h"
#include "JCUTAction.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;

namespace jcut {

int Interpreter::batchMode(CommonOptionsParser& OptionsParser) {
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

	return jcut::TotalTestsFailed;
}

void Interpreter::completionCallBack(const char * line, linenoiseCompletions *lc)
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

int Interpreter::mainLoop(CommonOptionsParser& OptionsParser) {
	cout << "Interpreter mode!" << endl;
	char *c_line = nullptr;
	string history_name = "jcut-history.txt";
	linenoiseHistoryLoad(history_name.c_str()); /* Load the history at startup */
	string line;
	const string prompt_input = "jcut $> ";
	const string prompt_more = "jcut ?> ";
	string prompt = prompt_input;
	linenoiseSetCompletionCallback(
			reinterpret_cast<linenoiseCompletionCallback*>(
					Interpreter::completionCallBack));
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

void Interpreter::processCommand(const string& cmd, CommonOptionsParser& OptionsParser)
{
	if (cmd.size() && cmd[0] == '/') {
		cout << "Unreconized command: "<< cmd << endl;;
	}
}


Interpreter::Interpreter() {

}

} /* namespace jcut */
