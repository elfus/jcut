//===-- jcut/TestRunnerVisitor.h --------------------------------*- C++ -*-===//
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

#ifndef TESTRUNNERVISITOR_H
#define	TESTRUNNERVISITOR_H

#include "TestParser.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include "OSRedirect.h"
#include <cstdlib>


using namespace tp;

class TestRunnerVisitor : public Visitor {
private:
    llvm::ExecutionEngine* mEE;
    std::vector<llvm::GenericValue> mArgs;//Dummy arguments
    bool mDumpFunctions;
    llvm::Module* mModule;
    // Temp vector to hold failed expected expressions
    std::vector<ExpectedExpression*> mExpExpr;
    /// Used to properly revert the mockup replacements for nested groups.
    /// Every time we enter in a group we store all of its MoclupFunctions
    /// in the stack. This used by individual tests and when returning to a
    /// different group.
    std::stack<llvm::Function*> mMockupRevert;
    // The order in which we will store the results.
    vector<ColumnName> mOrder;
    map<ColumnName,string> mColumnNames;

    void runFunction(LLVMFunctionHolder* FW);

    /// Executes the MockupFunctions stored in our stack, they are not discarded.
    void executeMockupFunctionsOnTopOfStack();

public:
    TestRunnerVisitor() = delete;
    TestRunnerVisitor(const TestRunnerVisitor& orig) = delete;
    TestRunnerVisitor(llvm::ExecutionEngine *EE, bool dump_func = false,
    		llvm::Module* mM=nullptr) : mEE(EE),
    		mDumpFunctions(dump_func), mModule(mM) {}
    virtual ~TestRunnerVisitor() { delete mEE; }

    bool isValidExecutionEngine() const { return mEE != nullptr; }
    void setColumnOrder(const vector<ColumnName>& order) { mOrder = order;}
    void setColumnNames(const map<ColumnName,string>& names) { mColumnNames = names;}
    void VisitGroupMockup(GlobalMockup *GM);

    void VisitGroupSetup(GlobalSetup *GS) {
        runFunction(GS);
    }

    void VisitGroupTeardown(GlobalTeardown *GT) {
        runFunction(GT);
    }

    // The cleanup
    void VisitTestGroup(TestGroup *TG);

    // ExpectedExpressions can be located either in any of after/before or
    // after_all/before_all statements
    void VisitExpectedExpression(ExpectedExpression* EE) {
    	mExpExpr.push_back(EE);
    }

    // The test definition
    void VisitTestDefinition(TestDefinition *TD);
};

#endif	/* TESTRUNNERVISITOR_H */

