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
	bool executeCommand(const std::string& cmd,
			CommonOptionsParser& OptionsParser, std::string& final_cmd);
	static void completionCallBack(const char * line, linenoiseCompletions *lc);

public:
	Interpreter() {}

	int batchMode(CommonOptionsParser& OptionsParser);
	int mainLoop(CommonOptionsParser& OptionsParser);
};

class Command {
	friend class Help;
protected:
	CommonOptionsParser& mOpp;
	std::vector<std::string> mArgs;
public:
	std::string name;
	std::string desc;
	Command(const Command&) = delete;
	Command(const std::string& n, const std::string& d,
			CommonOptionsParser& opp) : mOpp(opp), name(n), desc(d) {}
	virtual ~Command(){}

	void setArguments(const std::vector<std::string>& args) { mArgs = args; }
	virtual bool execute() {return true;}
	std::string str() const { return name;}
};

class Help : public Command{
public:
	Help(CommonOptionsParser& opp) : Command("/help",
			"Lists all the available options for the jcut interpreter.", opp) {}
	bool execute();
};

class Exit : public Command{
public:
	Exit(CommonOptionsParser& opp) : Command("/exit", "Exits the jcut interpreter.", opp) {}
};

class Pwd : public Command{
public:
	Pwd(CommonOptionsParser& opp) : Command("/pwd", "Prints the working directory where jcut is being run.", opp) {}
	bool execute();
};

class Load : public Command{
public:
	Load(CommonOptionsParser& opp) : Command("/load", "Loads all the specified source files separated by a space.", opp) {}
	bool execute() { return false;}
};

class Unload : public Command{
public:
	Unload(CommonOptionsParser& opp) : Command("/unload", "Removes the specified C source files from jcut memory.", opp) {}
	bool execute();
};

class Ls : public Command{
public:
	Ls(CommonOptionsParser& opp) : Command("/ls", "Lists all the functions for all the loaded C source files.", opp) {}
	bool execute();
};

class CommandFactory {
	friend class Help;
private:
	CommandFactory(CommonOptionsParser& opp) : mOpp(opp) {
		// Every time a new command is added make sure you add an entry here.
		registered_cmds["/help"] = unique_ptr<Command>(new Help(mOpp));
		registered_cmds["/exit"] = unique_ptr<Command>(new Exit(mOpp));
		registered_cmds["/pwd"] = unique_ptr<Command>(new Pwd(mOpp));
		registered_cmds["/ls"] = unique_ptr<Command>(new Ls(mOpp));
		registered_cmds["/load"] = unique_ptr<Command>(new Load(mOpp));
		registered_cmds["/unload"] = unique_ptr<Command>(new Unload(mOpp));
	}

	std::vector<std::string> parseArguments(const std::string&);
	std::map<std::string,unique_ptr<Command>> registered_cmds;
	static unique_ptr<CommandFactory> factory;
	CommonOptionsParser& mOpp;
public:
	static CommandFactory& instance(CommonOptionsParser& OptionsParser);
	Command* create(const string& cmd);
};

} /* namespace jcut */

#endif /* INTERPRETER_H_ */
