//===-- jcut/main.cpp -------------------------------------------*- C++ -*-===//
//
// Copyright (c) 2014 Adrián Ortega García
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
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

	// Initialize the JIT Engine only once
	llvm::InitializeNativeTarget();

	jcut::Interpreter interpreter(argc, argv);
	int return_code = 0;
	if(isTestFileProvided(argc, argv))
		return_code = interpreter.runAction<jcut::JCUTAction>(argc, argv);
	else
		return_code = interpreter.mainLoop();


	return return_code;
}

