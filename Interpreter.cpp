//===-- jcut/Interpreter.cpp ------------------------------------*- C++ -*-===//
//
// This file is part of JCUT, A Just-n-time C Unit Testing framework.
//
// Copyright (c) 2014 Adrián Ortega García <adrianog(dot)sw(at)gmail(dot)com>
// All rights reserved.
//
// JCUT is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// JCUT is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with JCUT (See LICENSE.TXT for details).
// If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//

#include <iostream>
#include <cstring>
#include <set>

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

Interpreter::Interpreter(const int argc, const char **argv)
: mArgc(argc), mArgv(nullptr), mOptionsParsed(false)
{
	mArgv = cloneArgv(argc, argv, mArgc);
	convertToAbsolutePaths(mArgc, mArgv);
	bool double_dash = false;
	bool compilation_db = false;
	for(int i = 0; i < mArgc; ++i) {
		string tmp(mArgv[i]);
		if(tmp == "-help" or tmp == "--help")
			runAction<clang::SyntaxOnlyAction>(mArgc, mArgv);
		if(tmp == "--")
			double_dash = true;
		if(tmp == "-p")
			compilation_db = true;
	}

	if(double_dash and compilation_db) {
		cerr << "Cannot process double dash and compilation data base. Provide only one." << endl;
		exit(EXIT_FAILURE);
	}

	if(!double_dash and !compilation_db) {
		cout << "Double dash not provided! Remember, if you want add your own compiler flags\n"
				"for the clang API you need to add a double dash '--' followed by the compiler flags."<< endl << endl;
		vector<string> backup;
		for(int i=0; i<mArgc; ++i)
			backup.push_back(mArgv[i]);
		backup.push_back("--");
		freeArgv(mArgc, mArgv);
		unsigned new_size = backup.size();
		const char ** out = new const char*[new_size];
		for(unsigned i=0; i<new_size; ++i) {
			out[i] = new char[backup[i].size()+1];
			memset(const_cast<char*>(out[i]), 0, backup[i].size()+1);
			memcpy(const_cast<char*>(out[i]), backup[i].c_str(), backup[i].size()+1);
		}
		mArgc = new_size;
		mArgv = out;
	}
}

Interpreter::~Interpreter() {
	freeArgv(mArgc, mArgv);
}

void Interpreter::convertToAbsolutePaths(int argc, const char **argv) {
	for(int i=0; i<argc; ++i) {
		if(llvm::sys::fs::is_regular_file(argv[i])) {
			SmallString<16> ss(StringRef(argv[i]));
			llvm::sys::fs::make_absolute(ss);
			string absolute = ss.str().str();
			delete [] argv[i];
			argv[i] = new char[absolute.size()+1];
			memset(const_cast<char*>(argv[i]), 0, absolute.size()+1);
			memcpy(const_cast<char*>(argv[i]),absolute.c_str(), absolute.size());
			const_cast<char*>(argv[i])[absolute.size()] = '\0';
		}
	}
}

static void printCopyRightNotice() {
	cout << "jcut Copyright (C) 2014  Adrian Ortega Garcia" << endl;
	cout << "This program comes with ABSOLUTELY NO WARRANTY;" << endl; // for details type `show w'.
	cout << "This is free software, and you are welcome to redistribute it" << endl;
	cout << "under certain conditions; See the LICENSE.TXT file for details. " << endl; // type `show c'
	cout << endl;
}

int Interpreter::mainLoop() {
	printCopyRightNotice();
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
	bool attempted = false;

	while(executed!="/exit" && (c_line = linenoise(prompt.c_str())) != nullptr) {
		unique_ptr<char> guard(c_line);// free memory a la C++ :)
		line = string(c_line);

		// Save it to the history
		if(line.size()) {
			linenoiseHistoryAdd(line.c_str());
			linenoiseHistorySave(history_name.c_str());
		}

		attempted = executeCommand(line, *this, executed);
		if(attempted)
			continue;


		if(linenoiseCtrlJPressed()) {
			prompt = prompt_more;
			if(line.empty()) {
				prompt = prompt_input;
				linenoiseCtrlJClear();
			}
		}


		jcut::JCUTAction::mInterpreterInput += line;
		if(prompt == prompt_input && !JCUTAction::mInterpreterInput.empty()) {
			if(!hasLoadedFiles()) {
				cout << "There are no loaded files. Try using the /load command." << endl;
				jcut::JCUTAction::mInterpreterInput.clear();
				continue;
			}

			runAction<JCUTAction>();
			jcut::JCUTAction::mInterpreterInput.clear();
		}
	}
	return 0;
}

void printArgv(int argc, const char** argv) {
	cout << "ARGUMENTS: ";
	for(int i=0; i<argc; ++i) {
		string tmp(argv[i]);
		cout << tmp << " ";
	}
	cout << endl;
}

template<class T>
int Interpreter::runAction() {
	int argc = 0;
	const char ** argv = cloneArgv(argc);
	int failed = runAction<T>(argc, argv);
	freeArgv(argc, argv);
	return failed;
}

template <class T>
int Interpreter::runAction(int argc, const char **argv) {
	// CommonOptionsParser constructor will parse arguments and create a
	// CompilationDatabase.  In case of error it will terminate the program.
	CommonOptionsParser OptionsParser(argc, argv);
	mOptionsParsed = true;

	// Use OptionsParser.getCompilations() and OptionsParser.getSourcePathList()
	// to retrieve CompilationDatabase and the list of input file paths.
	CompilationDatabase& CD = OptionsParser.getCompilations();
	// A clang tool can run over a number of sources in the same process...
	std::vector<std::string> Sources = OptionsParser.getSourcePathList();

	// Remove duplicates to correct a strange behavior:
	// The CommonOptionsParser uses a positional argument to detect the source files
	// and then builds a list with those files. The problem is that we run
	// this method many times and that list is not cleared, meaning that the
	// same source file is added over and over again. Thus executing the same
	// actions on the same file several times because it was repeated.
	// The workaround for this is just removing the duplicate source files.
	sort(Sources.begin(), Sources.end());
	Sources.erase(unique(Sources.begin(), Sources.end(), [&](const string& a, const string& b) {
		if(a.find(b) != string::npos)
			return true;
		if(b.find(a) != string::npos)
			return true;
		return false;
	}
	), Sources.end());
	// This is needed because the files we have removed are still contained in the
	// CommonOptionsParser variables.
	for(auto unloaded : unloadedFiles) {
		auto it = find(Sources.begin(), Sources.end(), unloaded);
		if(it != Sources.end())
			Sources.erase(it);
	}
	mLoadedFiles = Sources;

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

	FrontendActionFactory* generic_action = newFrontendActionFactory<T>();
	failed = Tool.run(generic_action);
	return failed;
}

const char** Interpreter::cloneArgv(int argc, const char** argv, int& new_argc) const
{
	vector<string> newCmdLine;
	const char** DoubleDash = std::find(argv, argv + argc, StringRef("--"));
	int middle_argc = DoubleDash - argv;
	int end_argc = (argv + argc) - DoubleDash;
	new_argc = 0;

	for(int i=0; i<middle_argc; ++i) {
		string tmp(argv[i]);
		if(mOptionsParsed) {
			if(tmp[0] == '-') {
				if(tmp == "-p")
					++i;
				continue;
			}
		}
		newCmdLine.push_back(tmp);
	}

	for(int i=0; i<end_argc; ++i)
		newCmdLine.push_back(DoubleDash[i]);


	const char ** out = new const char*[newCmdLine.size()];
	unsigned size = newCmdLine.size();
	for(unsigned i=0; i<size; ++i) {
		out[i] = new char[newCmdLine[i].size()+1];
		memset(const_cast<char*>(out[i]), 0, newCmdLine[i].size()+1);
		memcpy(const_cast<char*>(out[i]), newCmdLine[i].c_str(), newCmdLine[i].size()+1);
		++new_argc;
	}
	return out;
}

const char** Interpreter::cloneArgv(int& new_argc) const
{
	return cloneArgv(mArgc, mArgv, new_argc);
}

void Interpreter::freeArgv(int argc, const char** argv)
{
	for(int i=0; i<argc; ++i) {
		int size = sizeof(argv[i]);
		memset(const_cast<char*>(argv[i]),0, size);
		delete [] argv[i];
		argv[i] = nullptr;
	}
	delete [] argv;
}

bool Interpreter::addFileToArgv(const string& str)
{
	vector<string> backup;
	for(int i=0; i<mArgc; ++i)
		backup.push_back(mArgv[i]);
	auto it = backup.end();
	backup.insert(--it, str);

	auto found = find(unloadedFiles.begin(), unloadedFiles.end(), str);
	if(found != unloadedFiles.end())
		unloadedFiles.erase(found);

	freeArgv(mArgc, mArgv);

	unsigned new_size = backup.size();
	const char ** out = new const char*[new_size];
	for(unsigned i=0; i<new_size; ++i) {
		out[i] = new char[backup[i].size()+1];
		memset(const_cast<char*>(out[i]), 0, backup[i].size()+1);
		memcpy(const_cast<char*>(out[i]), backup[i].c_str(), backup[i].size()+1);
	}

	mArgc = new_size;
	mArgv = out;
	return true;
}

bool Interpreter::removeFileFromArgv(const string& str)
{
	vector<string> backup;
	for(int i=0; i<mArgc; ++i) {
		string arg(mArgv[i]);
		if(arg.find(str) == string::npos)
			backup.push_back(mArgv[i]);
		else
			unloadedFiles.push_back(mArgv[i]);
	}
	if(static_cast<unsigned>(mArgc) == backup.size())
		return false;

	cout << "File " << unloadedFiles.back() << " unloaded!" << endl;
	unsigned new_size = backup.size();

	freeArgv(mArgc, mArgv);

	const char ** out = new const char*[new_size];
	for(unsigned i=0; i<new_size; ++i) {
		out[i] = new char[backup[i].size()+1];
		memset(const_cast<char*>(out[i]), 0, backup[i].size()+1);
		memcpy(const_cast<char*>(out[i]), backup[i].c_str(), backup[i].size()+1);
	}

	mArgv = out;
	mArgc = new_size;
	return true;
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

/**
 *
 * @param[in] final_cmd Will contain the command executed
 *
 * @return true when attempted to execute a Command. The Command may or may not succeed.
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
			return true;
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

bool Load::execute() {
	if(mArgs.empty()) {
		cout << "You need to provide at least 1 file to load." << endl;
		return false;
	}

	for(const string& str : mArgs) {
		if(!llvm::sys::fs::is_regular_file(str)) {
			cerr << "Invalid file " << str << endl;
			continue;
		}
		// Make sure we convert it to its absolute path
		SmallString<16> ss(str);
		llvm::sys::fs::make_absolute(ss);
		string absolute = ss.str().str();
		if(!mInt.addFileToArgv(absolute))
			cerr << "Could not load file " << str << endl;
		else
			cout << "File " << absolute << " loaded succesfully." << endl;
	}
	return true;
}

bool Unload::execute() {
	if(mArgs.empty()) {
		cout << "No files were provided to unload" << endl;
		return false;
	}

	for(const string& str : mArgs) {
		if(!llvm::sys::fs::is_regular_file(str)) {
			cerr << "Invalid file " << str << endl;
			continue;
		}
		if(!mInt.removeFileFromArgv(str)) {
			cerr << "File " << str << " is NOT loaded. Try the /load command." << endl;
			cerr << "Type /help for more options." << endl;
		}
	}
	return true;
}

bool Ls::execute() {
	// 2 means, the binary name and the mythical --
	if(!mInt.hasLoadedFiles()) {
		cout << "There are no files loaded!" << endl;
		return true;
	}

	int failed = mInt.runAction<LsFunctionsAction>();
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
			if(str[i] == ' ')
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
			if(cmd.size() > c->name.size())
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
