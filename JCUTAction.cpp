//===-- jcut/JCUTAction.cpp - A ClangTool Action ----------------*- C++ -*-===//
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

#include "JCUTAction.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Frontend/CompilerInstance.h"

#include "TestParser.h"
#include "TestGeneratorVisitor.h"
#include "TestRunnerVisitor.h"
#include "TestLoggerVisitor.h"

using namespace llvm;

extern cl::opt<string> TestFileOpt;
extern cl::opt<bool> DumpOpt;

// Static variables from StdCapture class.
// @todo check if we can make them object variables.
int StdCapture::m_pipe[2];
int StdCapture::m_oldStdOut;
int StdCapture::m_oldStdErr;
bool StdCapture::m_capturing;
std::mutex StdCapture::m_mutex;
std::string StdCapture::m_captured;

namespace jcut {
#undef DEBUG_TYPE
#define DEBUG_TYPE "JCUTAction"

bool JCUTAction::mUseInterpreterInput;
std::string JCUTAction::mInterpreterInput;
// @todo remove this global variable.
int TotalTestsFailed = 0;

bool JCUTAction::BeginInvocation(CompilerInstance& CI) {
	DEBUG(errs() << "'JCUTAction' Beginning invocation\n");
	return true;
}

bool JCUTAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
	DEBUG(errs() << "'JCUTAction' BeginSourceFileAction on "
				 << Filename.str() << "\n");
	/* This is an important step: The backend needs to know which C source
	 * file to use, otherwise it fail saying that clang needs 1 positional
	 * argument.
	 */
	CI.getCodeGenOpts().BackendOptions.push_back(Filename.str());
	return true;
}

void JCUTAction::EndSourceFileAction() {
		DEBUG(errs() << "'JCUTAction' EndSourceFileAction\n");

		EmitLLVMOnlyAction::EndSourceFileAction();
		// The JIT Will take ownership of the Module!
		llvm::Module* module = takeModule();

		try {
			TestDriver driver;
			if(mUseInterpreterInput) {
				JCUTException::mExceptionSource = "jcut";
				driver.tokenize(mInterpreterInput.c_str());
			}
			else {
				JCUTException::mExceptionSource = TestFileOpt.getValue(); // quick workaround
				driver.tokenize(TestFileOpt.getValue());
			}
			unique_ptr<TestExpr> tests (driver.ParseTestExpr()); // Parse file and generate object structure tree

			DataPlaceholderVisitor dp;
			tests->accept(&dp); // Generate functions using data place holders.

			TestGeneratorVisitor visitor(module);
			tests->accept(&visitor); // Generate LLVM IR code

			std::string Error;
			TestRunnerVisitor runner(llvm::ExecutionEngine::createJIT(module, &Error),DumpOpt.getValue(),module);
			if (runner.isValidExecutionEngine() == false) {
				llvm::errs() << "unable to make execution engine: " << Error << "\n";
				return;
			}

			TestLoggerVisitor results_logger;
			results_logger.setLogFormat(TestLoggerVisitor::LOG_ALL);
			runner.setColumnOrder(results_logger.getColumnOrder());
			runner.setColumnNames(results_logger.getColumnNames());

			tests->accept(&runner);

			OutputFixerVisitor fixer(results_logger);
			// @note The following two calls have to happen in this exact
			// same order:
			//  1st OutputFixerVisitor so we can get the right widths of all the output
			//  2nd TestLoggerVisitor so we can print the test information
			tests->accept(&fixer);
			tests->accept(&results_logger);

			// this application exits with the number of tests failed.
			TotalTestsFailed += results_logger.getTestsFailed();
		} catch(const UnexpectedToken& e){
			errs() << e.what() << "\n";
		}
		catch (const JCUTException& e) {
			errs() << e.what() << "\n";
		}
	}

#undef DEBUG_TYPE
} /* namespace jcut */
