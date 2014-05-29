/**
 * @file Visitor.hxx
 * @author Adrian Ortega
 *
 * Created on May 1, 2014, 9:19 AM
 */

#ifndef VISITOR_HXX
#define	VISITOR_HXX

namespace tp { // tp stands for test parser

// Forward declarations
class FunctionCallExpr;
class Argument;
class VariableAssignmentExpr;
class BufferAlloc;
class FunctionArgument;
class Identifier;
class FunctionCallExpr;
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
class TestTeardownExpr;
class TestFunction;
class TestSetupExpr;
class TestFixtureExpr;
class MockupVariableExpr;
class MockupFunctionExpr;
class MockupFixtureExpr;
class TestMockupExpr;
class TestDefinitionExpr;
class TestGroup;
class UnitTestExpr;
class GlobalMockupExpr;
class GlobalSetupExpr;
class GlobalTeardownExpr;
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
    virtual void VisitVariableAssignmentExpr(VariableAssignmentExpr *) {}
    virtual void VisitBufferAlloc(BufferAlloc *) {}
    virtual void VisitIdentifier(Identifier *) {}
    virtual void VisitFunctionArgument(FunctionArgument *) {}
    virtual void VisitFunctionCallExpr(FunctionCallExpr *) {}
    virtual void VisitExpectedResult(ExpectedResult *) {}
    virtual void VisitExpectedConstant(ExpectedConstant *) {}
    virtual void VisitExpectedExpression(ExpectedExpression *) {}
    virtual void VisitConstant(Constant *) {}
    virtual void VisitNumericConstant(NumericConstant *) {}
    virtual void VisitStringConstant(StringConstant *) {}
    virtual void VisitCharConstant(CharConstant *) {}
    virtual void VisitComparisonOperator(ComparisonOperator *) {}
    virtual void VisitOperand(Operand *) {}
    virtual void VisitTestTeardowExpr(TestTeardownExpr *) {}
    virtual void VisitTestFunction(TestFunction *) {}
    virtual void VisitTestSetupExpr(TestSetupExpr *) {}
    virtual void VisitTestFixtureExpr(TestFixtureExpr *) {}
    virtual void VisitMockupVariableExpr(MockupVariableExpr *) {}
    virtual void VisitMockupFunctionExpr(MockupFunctionExpr *) {}
    virtual void VisitMockupFixtureExpr(MockupFixtureExpr *) {}
    virtual void VisitTestInfo(TestInfo* ) {}
    virtual void VisitTestMockupExpr(TestMockupExpr *) {}
    virtual void VisitTestDefinitionExpr(TestDefinitionExpr *) {}
    virtual void VisitTestGroup(TestGroup *) {}
    virtual void VisitUnitTestExpr(UnitTestExpr *) {}
    virtual void VisitGlobalMockupExpr(GlobalMockupExpr *) {}
    virtual void VisitGlobalSetupExpr(GlobalSetupExpr *) {}
    virtual void VisitGlobalTeardownExpr(GlobalTeardownExpr *) {}
    virtual void VisitTestFile(TestFile *) {}
    virtual ~Visitor() {}

    /*
     * The following methods visit each node in the object structure in a
     * pre-order fashion, meaning that all the current node is visited first
     * and then all its children.
     */
    virtual void VisitUnitTestFirst(UnitTestExpr *) {}
};

} // namespace tp

#endif	/* VISITOR_HXX */

