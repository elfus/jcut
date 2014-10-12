/*
 * Interpreter.h
 *
 *  Created on: Oct 12, 2014
 *      Author: aortegag
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <string>
#include "linenoise.h"

namespace clang{
	namespace tooling {
		class CommonOptionsParser;
	}
}

using clang::tooling::CommonOptionsParser;

namespace jcut {

class Interpreter {
private:
	void processCommand(const std::string& cmd, CommonOptionsParser& OptionsParser);
	static void completionCallBack(const char * line, linenoiseCompletions *lc);

public:
	Interpreter();

	int batchMode(CommonOptionsParser& OptionsParser);
	int mainLoop(CommonOptionsParser& OptionsParser);
};

} /* namespace jcut */

#endif /* INTERPRETER_H_ */
