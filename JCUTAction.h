//===-- jcut/JCUTAction.h - A ClangTool ACtion ------------------*- C++ -*-===//
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

#ifndef JCUTACTION_H_
#define JCUTACTION_H_

#include <string>
#include "clang/CodeGen/CodeGenAction.h"

namespace clang {
	class CompilerInstance;
}
namespace llvm {
	class StringRef;
}

using clang::CompilerInstance;
using llvm::StringRef;

namespace jcut {

extern int TotalTestsFailed;

class JCUTAction : public clang::EmitLLVMOnlyAction{
public:
	/* These two static variables are used as workaround to communicate with
	 * the main execution flow. Keep in mind that a JCUTAction will be
	 * created for each source file, thus its lifetime is less than that of
	 * the interpreter itself. Another thing is that we cannot get a direct
	 * handle to the JCUTAction object when it's created with the
	 * newFrontendActionFactory<T>() method. That's why we use these two
	 * static variables.
	 */
	static bool mUseInterpreterInput;
	static std::string mInterpreterInput;
	JCUTAction();

	bool BeginInvocation(CompilerInstance& CI);
	bool BeginSourceFileAction(CompilerInstance &CI, StringRef Filename);
	void EndSourceFileAction();
};

} /* namespace jcut */

#endif /* JCUTACTION_H_ */
