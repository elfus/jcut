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

	class Command {
	private:
		std::string name;
	public:
		Command(const Command&) = delete;
		Command(const std::string& n) : name(n) {}
		virtual ~Command(){}

		virtual bool execute() = 0;
		std::string str() const { return name;}
	};

	class Help : public Command{
	public:
		Help() : Command("help") {}
		bool execute() {
			cout << "jcut interpreter avilable commands:" << endl;
			return true;
		}
	};

	class CommandFactory {
	public:
		static Command* create(const string& cmd);
	};
public:
	Interpreter();

	int batchMode(CommonOptionsParser& OptionsParser);
	int mainLoop(CommonOptionsParser& OptionsParser);
};

} /* namespace jcut */

#endif /* INTERPRETER_H_ */
