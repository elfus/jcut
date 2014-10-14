//===-- jcut/TestLoggerVisitor.cpp - Logging facilities ---------*- C++ -*-===//
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
#include <TestLoggerVisitor.h>

void TestLoggerVisitor::logFunctionOutput(LLVMFunctionHolder *FH, const string& name)
{
	const vector<Warning>& warnings = FH->getWarnings();
	if(warnings.size()) {
		for(auto w : warnings)
			cout << w.what() << endl;
	}
	// If we want to print more information about a test, this is the place
	// for example we want print its output.
	if(FH->getOutput().size()) {
		stringstream ss;
		ss << "[" << name << " output]";
		cout << setw(WIDTH) << setfill('.') << '.' << setfill(' ') << endl;
		cout << setw(WIDTH) << right << ss.str() << left << endl;
		cout << FH->getOutput() << endl;
	}
}
