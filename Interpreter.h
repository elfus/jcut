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
	int mArgc;
	const char ** mArgv;
	vector<const char*> toBeFreed;

	void convertToAbsolutePaths(int argc, const char **argv);
	bool executeCommand(const std::string& cmd,
			Interpreter& i, std::string& final_cmd);
	static void completionCallBack(const char * line, linenoiseCompletions *lc);

public:
	Interpreter(const int argc, const char **argv);
	~Interpreter();

	// Runs the SyntaxOnlyAction followed by the Action defined by T.
	template<class T>
	int runAction(int argc, const char **argv);
	int mainLoop();
	int getArgc() const;
	// For every call to the cloneArgv() methods there has to be 1 call to freeArgv
	const char** cloneArgv() const;
	const char** cloneArgv(int argc, const char** argv) const;
	void freeArgv(int argc, const char** argv);
	bool removeFileFromArgv(const string& str);
};

class Command {
	friend class Help;
protected:
	Interpreter& mInt;
	std::vector<std::string> mArgs;
public:
	std::string name;
	std::string desc;
	Command(const Command&) = delete;
	Command(const std::string& n, const std::string& d,
			Interpreter& i) : mInt(i), name(n), desc(d) {}
	virtual ~Command(){}

	void setArguments(const std::vector<std::string>& args) { mArgs = args; }
	virtual bool execute() {return true;}
	std::string str() const { return name;}
};

class Help : public Command{
public:
	Help(Interpreter& i) : Command("/help",
			"Lists all the available options for the jcut interpreter.", i) {}
	bool execute();
};

class Exit : public Command{
public:
	Exit(Interpreter& i) : Command("/exit", "Exits the jcut interpreter.", i) {}
};

class Pwd : public Command{
public:
	Pwd(Interpreter& i) : Command("/pwd", "Prints the working directory where jcut is being run.", i) {}
	bool execute();
};

class Load : public Command{
public:
	Load(Interpreter& i) : Command("/load", "Loads all the specified source files separated by a space.",i) {}
	bool execute() { return false;}
};

class Unload : public Command{
public:
	Unload(Interpreter& i) : Command("/unload", "Removes the specified C source files from jcut memory.", i) {}
	bool execute();
};

class Ls : public Command{
public:
	Ls(Interpreter& i) : Command("/ls", "Lists all the functions for all the loaded C source files.", i) {}
	bool execute();
};

class CommandFactory {
	friend class Help;
private:
	CommandFactory(Interpreter& i) : mInterpreter(i) {
		// Every time a new command is added make sure you add an entry here.
		registered_cmds["/help"] = unique_ptr<Command>(new Help(i));
		registered_cmds["/exit"] = unique_ptr<Command>(new Exit(i));
		registered_cmds["/pwd"] = unique_ptr<Command>(new Pwd(i));
		registered_cmds["/ls"] = unique_ptr<Command>(new Ls(i));
		registered_cmds["/load"] = unique_ptr<Command>(new Load(i));
		registered_cmds["/unload"] = unique_ptr<Command>(new Unload(i));
	}

	std::vector<std::string> parseArguments(const std::string&);
	std::map<std::string,unique_ptr<Command>> registered_cmds;
	static unique_ptr<CommandFactory> factory;
	Interpreter& mInterpreter;
public:
	static CommandFactory& instance(Interpreter& i);
	Command* create(const string& cmd);
};

} /* namespace jcut */

#endif /* INTERPRETER_H_ */
