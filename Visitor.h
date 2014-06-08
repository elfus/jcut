/**
 * @file Visitor.h
 * @author Adrian Ortega
 *
 * Created on May 1, 2014, 9:19 AM
 */

#ifndef VISITOR_H
#define	VISITOR_H

namespace tp { // tp stands for test parser

// Forward declarations
class FunctionCall;
class Argument;
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
class TestInfo;
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
class UnitTests;
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
    virtual void VisitArgument(Argument *) {}
    virtual void VisitInitializerList(InitializerList *) {}
    virtual void VisitVariableAssignment(VariableAssignment *) {}
    virtual void VisitBufferAlloc(BufferAlloc *) {}
    virtual void VisitIdentifier(Identifier *) {}
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
    virtual void VisitTestTeardow(TestTeardown *) {}
    virtual void VisitTestFunction(TestFunction *) {}
    virtual void VisitTestSetup(TestSetup *) {}
    virtual void VisitTestFixture(TestFixture *) {}
    virtual void VisitMockupVariable(MockupVariable *) {}
    virtual void VisitMockupFunction(MockupFunction *) {}
    virtual void VisitMockupFixture(MockupFixture *) {}
    virtual void VisitTestInfo(TestInfo* ) {}
    virtual void VisitTestMockup(TestMockup *) {}
    virtual void VisitTestDefinition(TestDefinition *) {}
    virtual void VisitTestGroup(TestGroup *) {}
    virtual void VisitUnitTest(UnitTests *) {}
    virtual void VisitGlobalMockup(GlobalMockup *) {}
    virtual void VisitGlobalSetup(GlobalSetup *) {}
    virtual void VisitGlobalTeardown(GlobalTeardown *) {}
    virtual void VisitTestFile(TestFile *) {}
    virtual ~Visitor() {}

    /*
     * The following methods visit each node in the object structure in a
     * pre-order fashion, meaning that all the current node is visited first
     * and then all its children.
     */
    virtual void VisitUnitTestFirst(UnitTests *) {}
    virtual void VisitGlobalSetupFirst(GlobalSetup *) {}
    virtual void VisitGlobalTeardownFirst(GlobalTeardown *) {}
    virtual void VisitTestGroupFirst(TestGroup *) {}
};

} // namespace tp

#endif	/* VISITOR_H */

