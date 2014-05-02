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
class Identifier;
class FunctionCallExpr;
class TestTeardownExpr;
class TestFunction;
class TestSetupExpr;
class TestFixtureExpr;
class MockupVariableExpr;
class MockupFunctionExpr;
class MockupFixtureExpr;
class TestMockupExpr;
class TestDefinitionExpr;
class UnitTestExpr;
class GlobalMockupExpr;
class GlobalSetupExpr;
class GlobalTeardownExpr;
class TestFile;

class Visitor {
public:

    Visitor() {}
    Visitor(const Visitor& orig) = delete;

    /* All these methods return true when we want to keep visiting children
     * elements in the object structure, false to not visit children
     */
    virtual bool VisitArgument(Argument *) {return true;}
    virtual bool VisitVariableAssignmentExpr(VariableAssignmentExpr *) {return true;}
    virtual bool VisitBufferAlloc(BufferAlloc *) {return true;}
    virtual bool VisitIdentifier(Identifier *) {return true;}
    virtual bool VisitFunctionCallExpr(FunctionCallExpr *) {return true;}
    virtual bool VisitTestTeardowExpr(TestTeardownExpr *) {return true;}
    virtual bool VisitTestFunction(TestFunction *){ return true; }
    virtual bool VisitTestSetupExpr(TestSetupExpr *) {return true;}
    virtual bool VisitTestFixtureExpr(TestFixtureExpr *) {return true;}
    virtual bool VisitMockupVariableExpr(MockupVariableExpr *) {return true;}
    virtual bool VisitMockupFunctionExpr(MockupFunctionExpr *) {return true;}
    virtual bool VisitMockupFixtureExpr(MockupFixtureExpr *) {return true;}
    virtual bool VisitTestMockupExpr(TestMockupExpr *) {return true;}
    virtual bool VisitTestDefinitionExpr(TestDefinitionExpr *) {return true;}
    virtual bool VisitUnitTestExpr(UnitTestExpr *) {return true;}
    virtual bool VisitGlobalMockupExpr(GlobalMockupExpr *) {return true;}
    virtual bool VisitGlobalSetupExpr(GlobalSetupExpr *) {return true;}
    virtual bool VisitGlobalTeardownExpr(GlobalTeardownExpr *) {return true;}
    virtual bool VisitTestFile(TestFile *) {return true;}
    virtual ~Visitor() {}
};

} // namespace tp

#endif	/* VISITOR_HXX */

