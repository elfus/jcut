//===-- jcut/TestVisitor.h --------------------------------------*- C++ -*-===//
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

#ifndef VISITOR_H
#define	VISITOR_H

namespace tp { // tp stands for test parser

// Forward declarations
class FunctionCall;
class InitializerValue;
class InitializerList;
class VariableAssignment;
class BufferAlloc;
class FunctionArgument;
class Identifier;
class DesignatedInitializer;
class StructInitializer;
class FunctionCall;
class ExpectedResult;
class ExpectedConstant;
class ExpectedExpression;
class Constant;
class NumericConstant;
class StringConstant;
class CharConstant;
class ComparisonOperator;
class Operand;
class TestData;
class TestTeardown;
class TestFunction;
class TestSetup;
class TestFixture;
class MockupVariable;
class MockupFunction;
class MockupFixture;
class TestMockup;
class TestDefinition;
class TestGroup;
class GlobalMockup;
class GlobalSetup;
class GlobalTeardown;
class TestFile;

class Visitor {
public:

    Visitor() {}
    Visitor(const Visitor& orig) = delete;

    /*
     * The following methods visit each node in the object structure in a
     * post-order fashion, meaning that all the children of a given node are
     * visited first and then we visit the current node.
     */
    virtual void VisitInitializerList(InitializerList *) {}
    virtual void VisitVariableAssignment(VariableAssignment *) {}
    virtual void VisitBufferAlloc(BufferAlloc *) {}
    virtual void VisitIdentifier(Identifier *) {}
    virtual void VisitInitializerValue(InitializerValue *) {}
    virtual void VisitDesignatedInitializer(DesignatedInitializer *) {}
    virtual void VisitStructInitializer(StructInitializer *) {}
    virtual void VisitFunctionArgument(FunctionArgument *) {}
    virtual void VisitFunctionCall(FunctionCall *) {}
    virtual void VisitExpectedResult(ExpectedResult *) {}
    virtual void VisitExpectedConstant(ExpectedConstant *) {}
    virtual void VisitExpectedExpression(ExpectedExpression *) {}
    virtual void VisitConstant(Constant *) {}
    virtual void VisitNumericConstant(NumericConstant *) {}
    virtual void VisitStringConstant(StringConstant *) {}
    virtual void VisitCharConstant(CharConstant *) {}
    virtual void VisitComparisonOperator(ComparisonOperator *) {}
    virtual void VisitOperand(Operand *) {}
    virtual void VisitTestTeardown(TestTeardown *) {}
    virtual void VisitTestFunction(TestFunction *) {}
    virtual void VisitTestSetup(TestSetup *) {}
    virtual void VisitTestFixture(TestFixture *) {}
    virtual void VisitMockupVariable(MockupVariable *) {}
    virtual void VisitMockupFunction(MockupFunction *) {}
    virtual void VisitMockupFixture(MockupFixture *) {}
    virtual void VisitTestInfo(TestData* ) {}
    virtual void VisitTestMockup(TestMockup *) {}
    virtual void VisitTestDefinition(TestDefinition *) {}
    virtual void VisitTestGroup(TestGroup *) {}
    virtual void VisitGroupMockup(GlobalMockup *) {}
    virtual void VisitGroupSetup(GlobalSetup *) {}
    virtual void VisitGroupTeardown(GlobalTeardown *) {}
    virtual void VisitTestFile(TestFile *) {}
    virtual ~Visitor() {}

    /*
     * The following methods visit each node in the object structure in a
     * pre-order fashion, meaning that all the current node is visited first
     * and then all its children.
     */
    virtual void VisitFunctionCallFirst(FunctionCall *) {};
    virtual void VisitTestFileFirst(TestFile *) {}
    virtual void VisitGroupSetupFirst(GlobalSetup *) {}
    virtual void VisitGroupTeardownFirst(GlobalTeardown *) {}
    virtual void VisitTestDefinitionFirst(TestDefinition *) {}
    virtual void VisitTestGroupFirst(TestGroup *) {}
};

} // namespace tp

#endif	/* VISITOR_H */

