//===-- jcut/main.cpp -------------------------------------------*- C++ -*-===//
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
#include <string>
// For llvm::InitializeNativeTarget();
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "Interpreter.h"
#include "JCUTAction.h"

using namespace std;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory JcutOptions("JCUT Options");
// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

cl::opt<string> TestFileOpt("t", cl::Optional,  cl::ValueRequired, cl::desc("Input test file"), cl::value_desc("filename"));
cl::opt<bool> DumpOpt("dump", cl::init(false), cl::ZeroOrMore, cl::desc("Dump generated LLVM IR code"), cl::value_desc("filename"));
cl::opt<bool> NoForkOpt("no-fork", cl::init(false), cl::ZeroOrMore, cl::desc("Runs tests without fork()ing them"), cl::value_desc("filename"));

static bool isTestFileProvided(int argc, const char **argv) {
	bool provided = false;
	for(int i=0; i<argc; ++i) {
		string tmp(argv[i]);
		if(tmp == "-t") {
			provided = true;
			break;
		}
	}
	return provided;
}

int main(int argc, const char **argv, char * const *envp)
{
	TestFileOpt.setCategory(JcutOptions);
	DumpOpt.setCategory(JcutOptions);
	NoForkOpt.setCategory(JcutOptions);

	// Initialize the JIT Engine only once
	llvm::InitializeNativeTarget();

	jcut::Interpreter interpreter(argc, argv);
	int return_code = 0;
	if(isTestFileProvided(argc, argv))
		return_code = interpreter.runAction<jcut::JCUTAction>();
	else
		return_code = interpreter.mainLoop();


	return return_code;
}

