//===-- jcut/JCUTAction.cpp - A ClangTool ACtion ----------------*- C++ -*-===//
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

JCUTAction::JCUTAction() {
	// TODO Auto-generated constructor stub

}

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
			tests->accept(&runner);

			TestLoggerVisitor results_logger;
			OutputFixerVisitor fixer(results_logger);
			results_logger.setLogFormat(TestLoggerVisitor::LOG_ALL);
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
