/*
 * Interpreter.cpp
 *
 *  Created on: Oct 12, 2014
 *      Author: aortegag
 */

#include <iostream>
#include <cstring>

#include "llvm/Support/FileSystem.h"

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Interpreter.h"
#include "JCUTAction.h"
// used for accesing the exceptions, @todo move Exception classes to their own sourc file
#include "TestParser.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;

namespace jcut {

int Interpreter::batchMode(int argc, const char **argv) {
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

int Interpreter::cloneArgc() const
{
	return mArgc;
}

void printArgv(int argc, const char** argv) {
	cout << "COPY: ";
	for(int i=0; i<argc; ++i) {
		string tmp(argv[i]);
		cout << tmp << " ";
	}
	cout << endl;
}
const char** Interpreter::cloneArgv() const
{
	const char ** argv = new const char*[mArgc];
	for(int i=0; i<mArgc; ++i) {
		string tmp(mArgv[i]);
		argv[i] = new char[tmp.size()+1];
		memcpy(const_cast<char*>(argv[i]), tmp.c_str(), tmp.size()+1);
	}
	return argv;
}

void Interpreter::freeArgv(int argc, const char** argv)
{
	for(int i=0; i<argc; ++i) {
		delete [] argv[i];
		argv[i] = nullptr;
	}
	delete [] argv;
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

int Interpreter::mainLoop() {
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
	bool success = false;

	while(executed!="/exit" && (c_line = linenoise(prompt.c_str())) != nullptr) {
		unique_ptr<char> guard(c_line);// free memory a la C++ :)
		line = string(c_line);

		// Save it to the history
		if(line.size()) {
			linenoiseHistoryAdd(line.c_str());
			linenoiseHistorySave(history_name.c_str());
		}

		success = executeCommand(line, *this, executed);
		if(success)
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
			int argc = cloneArgc();
			const char** argv = cloneArgv();
			batchMode(argc, argv);
			freeArgv(argc, argv);
			jcut::JCUTAction::mInterpreterInput.clear();
		}
	}
	return 0;
}

/**
 *
 * @param[in] final_cmd Will contain the command executed
 *
 * @return true when cmd_str is a Command. The Command may or may not succeed.
 */
bool Interpreter::executeCommand(const string& cmd_str,
		Interpreter& i, std::string& final_cmd)
{
	final_cmd = "";

	if (cmd_str[0] == '/' && cmd_str.size()) {
		Command* cmd = CommandFactory::instance(i).create(cmd_str);

		if(!cmd) {
			cerr << "Unrecognized command: " << cmd_str
			<<". Try typing /help for a list of available commands." << endl;
			return false;
		}else {
			if(!cmd->execute())
				cerr << "Failed to execute command: " << cmd->str() << endl;
			final_cmd = cmd->name;
			return true;
		}
	}
	return false;
}

bool Help::execute() {
	cout << "jcut interpreter available commands:" << endl << endl;

	CommandFactory& f = CommandFactory::instance(mInt);
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

bool Unload::execute() {

	if(mArgs.empty())
		return false;
	cout << "Files to unload:" << endl;
	for(const string& str : mArgs) {
		cout << "\t" << str << endl;
	}
	return true;
}

bool Ls::execute() {
	int argc = mInt.cloneArgc();
	const char ** argv = mInt.cloneArgv();
	CommonOptionsParser OptionsParser(argc, argv);
	CompilationDatabase& CD = OptionsParser.getCompilations();
	std::vector<std::string> Sources = OptionsParser.getSourcePathList();
	ClangTool Tool(CD, Sources);
	FrontendActionFactory* ls_functions = newFrontendActionFactory<LsFunctionsAction>();
	int failed = Tool.run(ls_functions);
	mInt.freeArgv(argc, argv);
	if(failed)
		return false;
	return true;
}

unique_ptr<CommandFactory> CommandFactory::factory(nullptr);

vector<string> CommandFactory::parseArguments(const string& str) {
	vector<string> args;
	int quoted = 0;
	string tmp;
	unsigned i = str.find_first_of(" ");
	for(++i; i < str.size(); ++i) {
		if(str[i] == '"')
			++quoted;
		if(quoted == 2)
			quoted = 0;
		tmp += str[i];
		if((str[i] == ' ' || i == str.size()-1) && !quoted) {
			tmp = tmp.substr(0, tmp.size()-1);
			args.push_back(tmp);
			tmp.clear();
		}
	}
	if(quoted)
		throw JCUTException("Missing quotes for argument: "+tmp);
	return args;
}
CommandFactory& CommandFactory::instance(Interpreter& i)
{
	if(factory)
		return *(factory.get());
	factory = unique_ptr<CommandFactory>(new CommandFactory(i));
	return *(factory.get());
}

Command* CommandFactory::create(const string& cmd)
{
	// Some commands take arguments, i.e. load or unload, others don't
	const string c_ndx = cmd.substr(0, cmd.find_first_of(" "));
	auto it = registered_cmds.find(c_ndx);
	if(it != registered_cmds.end()) {
		Command* c = registered_cmds[c_ndx].get();
		vector<string> args;
		try {
			args = parseArguments(cmd);
		}catch(const JCUTException& e) {
			cerr << e.what() << endl;
			args.clear();
		}
		c->setArguments(args);
		return c;
	}
	return nullptr;
}

} /* namespace jcut */
