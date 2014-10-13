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

class Interpreter {
private:
	string executeCommand(const std::string& cmd, CommonOptionsParser& OptionsParser);
	static void completionCallBack(const char * line, linenoiseCompletions *lc);

public:
	Interpreter() {}

	int batchMode(CommonOptionsParser& OptionsParser);
	int mainLoop(CommonOptionsParser& OptionsParser);
};

class Command {
	friend class Help;
private:
	std::string name;
	std::string desc;
public:
	Command(const Command&) = delete;
	Command(const std::string& n, const std::string& d) : name(n), desc(d) {}
	virtual ~Command(){}

	virtual bool execute() {return true;}
	std::string str() const { return name;}
};

class Help : public Command{
public:
	Help() : Command("/help",
			"Lists all the available options for the jcut interpreter.") {}
	bool execute();
};

class Exit : public Command{
public:
	Exit() : Command("/exit", "Exits the jcut interpreter.") {}
};

class Pwd : public Command{
public:
	Pwd() : Command("/pwd", "Prints the working directory where jcut is being run.") {}
	bool execute();
};

class Load : public Command{
public:
	Load() : Command("/load", "Loads all the specified source files separated by a space.") {}
	bool execute() { return false;}
};

class Unload : public Command{
public:
	Unload() : Command("/unload", "Removes the specified C source files from jcut memory.") {}
	bool execute() { return false;}
};

class Ls : public Command{
public:
	Ls() : Command("/ls", "Lists all the functions for all the loaded C source files.") {}
	bool execute();
};

class CommandFactory {
	friend class Help;
private:
	CommandFactory() {
		// Every time a new command is added make sure you add an entry here.
		registered_cmds["/help"] = unique_ptr<Command>(new Help);
		registered_cmds["/exit"] = unique_ptr<Command>(new Exit);
		registered_cmds["/pwd"] = unique_ptr<Command>(new Pwd);
		registered_cmds["/ls"] = unique_ptr<Command>(new Ls);
		registered_cmds["/load"] = unique_ptr<Command>(new Load);
		registered_cmds["/unload"] = unique_ptr<Command>(new Unload);
	}

	std::map<std::string,unique_ptr<Command>> registered_cmds;
	static unique_ptr<CommandFactory> factory;
public:
	static CommandFactory& instance();
	Command* create(const string& cmd);
};

} /* namespace jcut */

#endif /* INTERPRETER_H_ */
