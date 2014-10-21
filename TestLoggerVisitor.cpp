//===-- jcut/TestLoggerVisitor.cpp - Logging facilities ---------*- C++ -*-===//
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
