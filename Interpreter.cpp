/*
 * Interpreter.cpp
 *
 *  Created on: Oct 12, 2014
 *      Author: aortegag
 */

#include <iostream>

#include "llvm/Support/FileSystem.h"

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
	cout << "jcut interpreter!" << endl;
	cout << "For a complete list of available commands type /help" << endl << endl;
	char * c_line = nullptr;
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
	string executed = "";

	while(executed!="/exit" && (c_line = linenoise(prompt.c_str())) != nullptr) {
		unique_ptr<char> guard(c_line);// free memory a la C++ :)
		line = string(c_line);

		// Save it to the history
		if(line.size()) {
			linenoiseHistoryAdd(line.c_str());
			linenoiseHistorySave(history_name.c_str());
		}

		executed = executeCommand(line,OptionsParser);
		if(!executed.empty())
			continue;

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
	}
	return 0;
}

// Returns a string containing the command executed. Empty string when no command was executed
string Interpreter::executeCommand(const string& cmd_str, CommonOptionsParser& OptionsParser)
{
	if (cmd_str[0] == '/' && cmd_str.size()) {
		Command* cmd = CommandFactory::instance().create(cmd_str);
		if(!cmd) {
			cerr << "Unrecognized command: " << cmd_str
			<<". Try typing /help for a list of available commands." << endl;
		}else if(!cmd->execute())
			cerr << "Failed to execute command: " << cmd->str() << endl;
		return cmd_str;
	}
	return "";
}

bool Help::execute() {
	cout << "jcut interpreter available commands:" << endl << endl;

	CommandFactory& f = CommandFactory::instance();
	for(auto it = f.registered_cmds.begin(); it != f.registered_cmds.end(); ++it) {
		const unique_ptr<Command>& p = (*it).second;
		cout << "\t" << p->name << " - " << p->desc << endl;
	}
	cout << endl;
	return true;
}

bool Pwd::execute() {
	SmallVector<char,64> pwd;
	llvm::sys::fs::current_path(pwd);
	for(auto c : pwd)
		cout << c;
	cout << endl;
	return true;
}

bool Ls::execute() {
	return false;
}

CommandFactory CommandFactory::factory;

CommandFactory& CommandFactory::instance()
{
	return factory;
}

Command* CommandFactory::create(const string& cmd)
{
	auto it = registered_cmds.find(cmd);
	if(it != registered_cmds.end())
		return registered_cmds[cmd].get();
	return nullptr;
}

} /* namespace jcut */
