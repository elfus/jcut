//===-- jcut/JCUTAction.h - A ClangTool Action ------------------*- C++ -*-===//
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

#ifndef JCUTACTION_H_
#define JCUTACTION_H_

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace clang {
	class CompilerInstance;
}
namespace llvm {
	class StringRef;
}

using namespace clang;
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
	JCUTAction() {}

	bool BeginInvocation(CompilerInstance& CI);
	bool BeginSourceFileAction(CompilerInstance &CI, StringRef Filename);
	void EndSourceFileAction();
};

/////////
class LsFunctionsVisitor : public RecursiveASTVisitor<LsFunctionsVisitor> {
private:
	std::string mCurrentFile;
public:
	LsFunctionsVisitor(const std::string str) : mCurrentFile(str) {}
	virtual ~LsFunctionsVisitor() {}
	virtual bool VisitFunctionDecl(FunctionDecl* D){
		SourceLocation sl = D->getLocation();
		SourceManager& m = D->getASTContext().getSourceManager();
		std::string source_location = m.getFilename(sl).str();
		if(mCurrentFile == source_location) {
			std::stringstream ss;
			ss << "\t" << D->getCallResultType().getAsString() << " ";
			ss << "\t" << D->getQualifiedNameAsString() << "(";
			FunctionDecl::param_iterator i = nullptr;
			for( i = D->param_begin(); i !=D->param_end(); ++i) {
				ss << (*i)->getType().getAsString() << " ";
				ss << (*i)->getFirstDecl()->getNameAsString() << ", ";
			}
			std::string str = ss.str();
			str = str.substr(0,str.find_last_of(","));
			str += ");";
			std::cout << str << std::endl;
		}
		return true;
	}
};

class LsFunctionsConsumer : public ASTConsumer {
public:
	LsFunctionsConsumer(const std::string s) {
		visitor = std::unique_ptr<LsFunctionsVisitor>(new LsFunctionsVisitor(s));
	}
	virtual ~LsFunctionsConsumer() {}
	virtual void HandleTranslationUnit(ASTContext& C) {
		visitor->TraverseTranslationUnitDecl(C.getTranslationUnitDecl());
		std::cout << std::endl;
	}
private:
	std::unique_ptr<LsFunctionsVisitor> visitor;
};

class LsFunctionsAction : public ASTFrontendAction {
	virtual ASTConsumer* CreateASTConsumer(
			clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
		std::cout << InFile.str() << ":" << std::endl;
		return new LsFunctionsConsumer(InFile.str());
	}
};
////////

} /* namespace jcut */

#endif /* JCUTACTION_H_ */
