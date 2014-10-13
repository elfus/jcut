/*
 * Interpreter.h
 *
 *  Created on: Oct 12, 2014
 *      Author: aortegag
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <iostream>
#include <string>
#include <map>
#include "linenoise.h"

namespace clang{
	namespace tooling {
		class CommonOptionsParser;
	}
}

using clang::tooling::CommonOptionsParser;
using namespace std;

namespace jcut {

// @todo Keep a copy of the original command line!, argc and argv
class Interpreter {
private:
	const int mArgc;
	const char ** mArgv;
	bool executeCommand(const std::string& cmd,
			const int argc, const char **argv, std::string& final_cmd);
	static void completionCallBack(const char * line, linenoiseCompletions *lc);

public:
	Interpreter(const int argc, const char **argv) : mArgc(argc), mArgv(argv) {}

	int batchMode(int &argc, const char **argv);
	int mainLoop(int &argc, const char **argv);
	int cloneArgc();
	const char** cloneArgv();
};

class Command {
	friend class Help;
protected:
	int mArgc;
	const char ** mArgv;
	std::vector<std::string> mArgs;
public:
	std::string name;
	std::string desc;
	Command(const Command&) = delete;
	Command(const std::string& n, const std::string& d,
			const int &argc, const char **argv) : mArgc(argc), mArgv(argv), name(n), desc(d) {}
	virtual ~Command(){}

	void setArguments(const std::vector<std::string>& args) { mArgs = args; }
	virtual bool execute() {return true;}
	std::string str() const { return name;}
};

class Help : public Command{
public:
	Help(int &argc, const char **argv) : Command("/help",
			"Lists all the available options for the jcut interpreter.", argc, argv) {}
	bool execute();
};

class Exit : public Command{
public:
	Exit(int &argc, const char **argv) : Command("/exit", "Exits the jcut interpreter.", argc, argv) {}
};

class Pwd : public Command{
public:
	Pwd(int &argc, const char **argv) : Command("/pwd", "Prints the working directory where jcut is being run.", argc, argv) {}
	bool execute();
};

class Load : public Command{
public:
	Load(int &argc, const char **argv) : Command("/load", "Loads all the specified source files separated by a space.", argc, argv) {}
	bool execute() { return false;}
};

class Unload : public Command{
public:
	Unload(int &argc, const char **argv) : Command("/unload", "Removes the specified C source files from jcut memory.", argc, argv) {}
	bool execute();
};

class Ls : public Command{
public:
	Ls(int &argc, const char **argv) : Command("/ls", "Lists all the functions for all the loaded C source files.", argc, argv) {}
	bool execute();
};

class CommandFactory {
	friend class Help;
private:
	CommandFactory(int &argc, const char **argv) : mArgc(argc), mArgv(argv) {
		// Every time a new command is added make sure you add an entry here.
		registered_cmds["/help"] = unique_ptr<Command>(new Help(mArgc, mArgv));
		registered_cmds["/exit"] = unique_ptr<Command>(new Exit(mArgc, mArgv));
		registered_cmds["/pwd"] = unique_ptr<Command>(new Pwd(mArgc, mArgv));
		registered_cmds["/ls"] = unique_ptr<Command>(new Ls(mArgc, mArgv));
		registered_cmds["/load"] = unique_ptr<Command>(new Load(mArgc, mArgv));
		registered_cmds["/unload"] = unique_ptr<Command>(new Unload(mArgc, mArgv));
	}

	std::vector<std::string> parseArguments(const std::string&);
	std::map<std::string,unique_ptr<Command>> registered_cmds;
	static unique_ptr<CommandFactory> factory;
	int mArgc;
	const char ** mArgv;
public:
	static CommandFactory& instance(int &argc, const char **argv);
	Command* create(const string& cmd);
};

} /* namespace jcut */

#endif /* INTERPRETER_H_ */
