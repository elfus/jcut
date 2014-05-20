/*
 * File:   TestParser.hxx
 * Author: aortegag
 *
 * Created on April 17, 2014, 10:09 AM
 */

#ifndef TESTPARSER_HXX
#define	TESTPARSER_HXX

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include "Visitor.hxx"

using namespace std;

class Exception : public std::exception {
public:
    Exception() = delete;

    Exception(const string& msg) : mMsg(msg) {}

    const char* what() const throw () {
        return mMsg.c_str();
    }
private:
    string mMsg;
};

namespace tp { // tp stands for test parser
    
class Tokenizer {
public:
    enum Token {
        TOK_ERR = -1,
        TOK_EOF = -2,
        TOK_ASCII_CHAR = -3,
        TOK_IDENTIFIER = -4, // An identifier is a string representing a function or variable name
        TOK_EQ_OP = -5,
        TOK_INT = -6,
        TOK_DOUBLE = -7, // Will handle floats too
        TOK_CHAR = -8,
        TOK_STRING = -9,
        TOK_BUFF_ALLOC = -10,
        TOK_ARRAY_INIT = -11,
        TOK_COMPARISON_OP = -12,
        // keywords
        TOK_BEFORE = -100,
        TOK_AFTER = -101,
        TOK_MOCKUP = -102,
        TOK_BEFORE_ALL = -103,
        TOK_AFTER_ALL = -104,
        TOK_MOCKUP_ALL = -105,
    };

    enum Identifier {
        ID_UNKOWN = 0,
        ID_FUNCTION,
        ID_VARIABLE,
    };

    Tokenizer(const string& filename);

    ~Tokenizer() {}

    Token currentToken() {
        return mCurrentToken;
    }

    int nextToken();

    int getInteger() { return mInt; }

    double getDouble() { return mDouble; }

    string getBuffAlloc() { return mBuffAlloc;  }

    string getTokenStringValue() { return mTokStrValue; }

    Identifier getIdentifierType() {  return mIdType;  }

    bool isCharIgnored(char);

private:
    std::ifstream mInput;
    Token mCurrentToken;
    string mFunction;
    char mEqOp;
    int mInt;
    double mDouble;
    string mBuffAlloc;
    string mTokStrValue;
    char mLastChar;
    Identifier mIdType;
};

class TestExpr {
public:

    TestExpr() { ++leaks; }

    virtual void dump() = 0;
    
    virtual void accept(Visitor *) = 0;

    virtual ~TestExpr() {--leaks;}

    static int leaks;
};

class ComparisonOperator : public TestExpr {
public:
    enum Type {
        EQUAL_TO,
        NOT_EQUAL_TO,
        GREATER_OR_EQUAL,
        LESS_OR_EQUAL,
        GREATER,
        LESS,
        INVALID
    };

    ComparisonOperator(const string& str) : mStringRepresentation(str),
    mType(INVALID) {
        if (mStringRepresentation == "==")
            mType = EQUAL_TO;
        else if (mStringRepresentation == "!=")
            mType = NOT_EQUAL_TO;
        else if (mStringRepresentation == ">=")
            mType = GREATER_OR_EQUAL;
        else if (mStringRepresentation == "<=")
            mType = LESS_OR_EQUAL;
        else if (mStringRepresentation == ">")
            mType = GREATER;
        else if (mStringRepresentation == "<")
            mType = LESS;
    }
    
    void dump() {
        cout << mStringRepresentation;
    }
    
    void accept(Visitor* v) {
        v->VisitComparisonOperator(this);
    }
private:
    string mStringRepresentation;
    Type mType;
};

class Argument : public TestExpr {
protected:
    string StringRepresentation;
    Tokenizer::Token TokenType;
public:

    Argument(const string& str, Tokenizer::Token token) :
    StringRepresentation(str), TokenType(token) { }

    ~Argument() {}
    
    const string& getStringRepresentation() const { return StringRepresentation; }
    Tokenizer::Token getTokenType() const { return TokenType; }

    void dump() {
        cout << StringRepresentation;
    }
    
    void accept(Visitor *v) {
       v->VisitArgument(this); // Argument doesn't have any children
    }
};

class Identifier : public TestExpr {
private:
    string mIdentifierString;
public:

    Identifier(const string& str) : mIdentifierString(str) {    }

    ~Identifier() {    }

    void dump() {
        cout << mIdentifierString;
    }
    
    void accept(Visitor *v) {
        v->VisitIdentifier(this);
    }
    
    string getIdentifierStr() const {return mIdentifierString;}
};

class FunctionArgument;

class FunctionCallExpr : public TestExpr {
private:
    Identifier *FunctionName;
    vector<FunctionArgument*> FunctionArguments;
public:
    
    Identifier *getIdentifier() const {return FunctionName;}

    FunctionCallExpr(Identifier* name, const vector<FunctionArgument*>& arg);

    virtual ~FunctionCallExpr();

    void dump();
    
    void accept(Visitor *v);
};

class ExpectedResult : public TestExpr {
private:
    ComparisonOperator* mCompOp;
    Argument* mArgument;
public:
    ExpectedResult(ComparisonOperator* cmp, Argument* Arg) :
    mCompOp(cmp), mArgument(Arg) {}
    ~ExpectedResult() {
        delete mArgument;
        delete mCompOp;
    }
    void dump() {
        mCompOp->dump();
        mArgument->dump();
    }
    
    void accept(Visitor *v) {
        mCompOp->accept(v);
        mArgument->accept(v);
        v->VisitExpectedResult(this);
    }
    
    ComparisonOperator* getComparisonOperator() const { return mCompOp; }
    Argument* getArgument() const { return mArgument; }
};

class VariableAssignmentExpr : public TestExpr {
private:
    Identifier *mIdentifier;
    Argument *mArgument;
public:

    VariableAssignmentExpr(Identifier *id, Argument *arg) :
    mIdentifier(nullptr), mArgument(nullptr) {
        mIdentifier = dynamic_cast<Identifier*> (id);
        if (mIdentifier == nullptr)
            throw Exception("Invalid Identifier type");

        mArgument = dynamic_cast<Argument*> (arg);
        if (mArgument == nullptr)
            throw Exception("Invalid Argument type");
    }

    ~VariableAssignmentExpr() {
        delete mIdentifier;
        delete mArgument;
    }
    
    Identifier* getIdentifier() const { return mIdentifier; }
    Argument* getArgument() const { return mArgument; }

    void dump() {
        mIdentifier->dump();
        cout << " = ";
        mArgument->dump();
    }
    
    void accept(Visitor *v){
        mIdentifier->accept(v);
        mArgument->accept(v);
        v->VisitVariableAssignmentExpr(this);
    }
};

class MockupVariableExpr : public TestExpr {
private:
    VariableAssignmentExpr *mVariableAssignment;
public:

    MockupVariableExpr(VariableAssignmentExpr *var) : mVariableAssignment(nullptr) {
        mVariableAssignment = dynamic_cast<VariableAssignmentExpr*> (var);
        if (mVariableAssignment == nullptr)
            throw Exception("Invalid Variable Assignment Expression type");
    }

    ~MockupVariableExpr() {
        if (mVariableAssignment)
            delete mVariableAssignment;
    }

    void dump() {
        mVariableAssignment->dump();
    }
    
    void accept(Visitor *v) {
        mVariableAssignment->accept(v);
        v->VisitMockupVariableExpr(this);
    }
};

class MockupFunctionExpr : public TestExpr {
private:
    FunctionCallExpr *mFunctionCall;
    Argument *mArgument;
public:

    MockupFunctionExpr(FunctionCallExpr *call, Argument *arg) :
    mFunctionCall(nullptr), mArgument(nullptr) {
        mFunctionCall = dynamic_cast<FunctionCallExpr*> (call);
        if (mFunctionCall == nullptr)
            throw Exception("Invalid FunctionCallExpr type");

        mArgument = dynamic_cast<Argument*> (arg);
        if (mArgument == nullptr)
            throw Exception("Invalid Argument type");
    }

    ~MockupFunctionExpr() {
        if (mFunctionCall)
            delete mFunctionCall;

        if (mArgument)
            delete mArgument;
    }

    void dump() {
        mFunctionCall->dump();
        cout << " = ";
        mArgument->dump();
    }
    
    void accept(Visitor *v) {
        mFunctionCall->accept(v);
        v->VisitMockupFunctionExpr(this);
    }
};

class MockupFixtureExpr : public TestExpr {
private:
    vector<MockupFunctionExpr*> mMockupFunctions;
    vector<MockupVariableExpr*> mMockupVariables;
public:
    MockupFixtureExpr(const vector<MockupFunctionExpr*>& func,
            const vector<MockupVariableExpr*>& var) :
    mMockupFunctions(func), mMockupVariables(var) {
    }

    ~MockupFixtureExpr() {
        for (auto*& ptr : mMockupFunctions)
            delete ptr;

        for (auto*& ptr : mMockupVariables)
            delete ptr;
    }

    void dump() {
        if (mMockupVariables.size())
            cout << "MOCKUP VARIABLES" << endl;
        for (auto*& ptr : mMockupVariables) {
            ptr->dump();
            cout << endl;
        }

        if (mMockupFunctions.size())
            cout << "MOCKUP FUNCTIONS" << endl;
        for (auto*& ptr : mMockupFunctions) {
            ptr->dump();
            cout << endl;
        }
    }
    
    void accept(Visitor *v) {
        for (auto*& ptr : mMockupVariables)
            ptr->accept(v);

        for (auto*& ptr : mMockupFunctions)
            ptr->accept(v);
        
        v->VisitMockupFixtureExpr(this);
    }
};

class TestMockupExpr : public TestExpr {
private:
    MockupFixtureExpr *mMockupFixture;
public:

    TestMockupExpr(MockupFixtureExpr *fixt) : mMockupFixture(fixt) {
    }

    ~TestMockupExpr() {
        if (mMockupFixture) delete mMockupFixture;
    }

    void dump() {
        cout << "mockup {" << endl;
        mMockupFixture->dump();
        cout << "}" << endl;
    }
    
    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitTestMockupExpr(this);
    }
};

class TestFixtureExpr : public TestExpr {
private:
    vector<FunctionCallExpr*> mFunctionCalls;
    vector<VariableAssignmentExpr*> mVarAssign;
public:

    TestFixtureExpr(const vector<FunctionCallExpr*>& func,
            const vector<VariableAssignmentExpr*>& var) :
    mFunctionCalls(func), mVarAssign(var) {

    }

    ~TestFixtureExpr() {
        for (auto*& ptr : mFunctionCalls)
            delete ptr;

        for (auto*& ptr : mVarAssign)
            delete ptr;
    }

    void dump() {
        if (mVarAssign.size()) {
            cout << endl << "VARIABLE ASSIGNMENT" << endl;
            for (auto*& ptr : mVarAssign) {
                ptr->dump();
                cout << " ";
            }
            cout << endl;
        }

        if (mFunctionCalls.size()) {
            cout << endl << "FUNCTION CALLS" << endl;
            for (auto*& ptr : mFunctionCalls) {
                ptr->dump();
                cout << " ";
            }
            cout << endl;
        }
    }
    
    void accept(Visitor *v) {
        for (auto*& ptr : mVarAssign)
            ptr->accept(v);
        
        for (auto*& ptr : mFunctionCalls)
            ptr->accept(v);        

        v->VisitTestFixtureExpr(this);
    }
};

class TestSetupExpr : public TestExpr {
private:
    TestFixtureExpr* mTestFixtureExpr;
public:

    TestSetupExpr(TestFixtureExpr* fixture) : mTestFixtureExpr(fixture) {
    }

    ~TestSetupExpr() {
        if (mTestFixtureExpr) delete mTestFixtureExpr;
    }

    void dump() {
        cout << "before {";
        mTestFixtureExpr->dump();
        cout << "}" << endl;
    }
    
    void accept(Visitor *v) {
        mTestFixtureExpr->accept(v);
        v->VisitTestSetupExpr(this);
    }
};

class TestFunction : public TestExpr {
private:
    FunctionCallExpr* mFunctionCall;
    ExpectedResult* mExpectedResult;
public:
    TestFunction(FunctionCallExpr *F, ExpectedResult* E=nullptr) : mFunctionCall(F),
            mExpectedResult(E) {}
    ~TestFunction() {
        if(mFunctionCall) delete mFunctionCall;
        if(mExpectedResult) delete mExpectedResult;
    }
    
    FunctionCallExpr* getFunctionCall() const {return mFunctionCall; }
    ExpectedResult* getExpectedResult() const {return mExpectedResult; }
    
    void dump() {
        mFunctionCall->dump();
        mExpectedResult->dump();
    }
    
    void accept(Visitor *v) {
        mFunctionCall->accept(v);
        if (mExpectedResult)
            mExpectedResult->accept(v);
        v->VisitTestFunction(this);
    }
};

class TestTeardownExpr : public TestExpr {
private:
    TestFixtureExpr* mTestFixture;
public:

    TestTeardownExpr(TestFixtureExpr* fixture) : mTestFixture(fixture) {
    }

    ~TestTeardownExpr() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "after {";
        mTestFixture->dump();
        cout << "}" << endl << endl;
    }
    
    void accept(Visitor *v) {
        mTestFixture->accept(v);
        v->VisitTestTeardowExpr(this);
    }
};

class TestDefinitionExpr : public TestExpr {
private:
    TestFunction *FunctionCall;
    TestSetupExpr *TestSetup;
    TestTeardownExpr *TestTeardown;
    TestMockupExpr *TestMockup;

public:

    TestDefinitionExpr(TestFunction *function,
            TestSetupExpr *setup = nullptr,
            TestTeardownExpr *teardown = nullptr,
            TestMockupExpr *mockup = nullptr) :
    FunctionCall(function), TestSetup(setup),
    TestTeardown(teardown), TestMockup(mockup) {
    }
    
    TestFunction * getTestFunction() const {return FunctionCall;}

    virtual ~TestDefinitionExpr() {
        if (FunctionCall) delete FunctionCall;
        if (TestSetup) delete TestSetup;
        if (TestTeardown) delete TestTeardown;
        if (TestMockup) delete TestMockup;
    }

    void dump() {
        if (TestMockup) {
            TestMockup->dump();
        }
        if (TestSetup) {
            TestSetup->dump();
        }
        FunctionCall->dump();
        cout << endl;
        if (TestTeardown) {
            TestTeardown->dump();
        }
    }
    
    void accept(Visitor *v) {
        if(TestMockup) TestMockup->accept(v);
        if(TestSetup) TestSetup->accept(v);
        FunctionCall->accept(v);
        if(TestTeardown) TestTeardown->accept(v);
        v->VisitTestDefinitionExpr(this);
    }
};

class UnitTestExpr : public TestExpr {
private:
    vector<TestDefinitionExpr*> TestDefinitions;
public:

    UnitTestExpr(const vector<TestDefinitionExpr*>& def) : TestDefinitions(def) { }

    virtual ~UnitTestExpr() {
        for (auto*& ptr : TestDefinitions)
            delete ptr;
    }

    void dump() {
        for (auto*& ptr : TestDefinitions) {
            ptr->dump();
            cout << endl;
        }
    }
    
    void accept(Visitor *v) {
        for (auto*& ptr : TestDefinitions) {
            ptr->accept(v);
        }
        v->VisitUnitTestExpr(this);
    }
};

class GlobalMockupExpr : public TestExpr {
private:
    MockupFixtureExpr *mMockupFixture;
public:

    GlobalMockupExpr(MockupFixtureExpr *fixt) : mMockupFixture(fixt) {    }

    ~GlobalMockupExpr() {
        if (mMockupFixture) delete mMockupFixture;
    }

    void dump() {
        cout << "mockup_all {" << endl;
        mMockupFixture->dump();
        cout << "}" << endl << endl;
    }

    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitGlobalMockupExpr(this);
    }
};

class GlobalSetupExpr : public TestExpr {
private:
    TestFixtureExpr *mTestFixture;
public:

    GlobalSetupExpr(TestFixtureExpr *fixt) : mTestFixture(fixt) {    }

    ~GlobalSetupExpr() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "before_all {";
        mTestFixture->dump();
        cout << "}" << endl << endl;
    }
    
    void accept(Visitor *v) {
        mTestFixture->accept(v);
        v->VisitGlobalSetupExpr(this);
    }
};

class GlobalTeardownExpr : public TestExpr {
private:
    TestFixtureExpr *mTestFixture;
public:

    GlobalTeardownExpr(TestFixtureExpr *fixt) : mTestFixture(fixt) {
    }

    ~GlobalTeardownExpr() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "after_all {" << endl;
        mTestFixture->dump();
        cout << "}" << endl;
    }
    
    void accept(Visitor *v) {
        mTestFixture->accept(v);
        v->VisitGlobalTeardownExpr(this);
    }
};

class TestFile : public TestExpr {
private:
    GlobalMockupExpr *mGlobalMockup;
    GlobalSetupExpr *mGlobalSetup;
    GlobalTeardownExpr *mGlobalTeardown;
    UnitTestExpr *mUnitTest;
public:

    TestFile(UnitTestExpr *ut, GlobalMockupExpr *gm = nullptr,
            GlobalSetupExpr *gs = nullptr, GlobalTeardownExpr *gt = nullptr) :
    mGlobalMockup(gm), mGlobalSetup(gs), mGlobalTeardown(gt),
    mUnitTest(ut) {
        
    }

    ~TestFile() {
        if (mGlobalMockup) delete mGlobalMockup;
        if (mGlobalSetup) delete mGlobalSetup;
        if (mGlobalTeardown) delete mGlobalTeardown;
        if (mUnitTest) delete mUnitTest;
    }

    void dump() {
        if (mGlobalMockup) mGlobalMockup->dump();
        if (mGlobalSetup) mGlobalSetup->dump();
        if (mGlobalTeardown) mGlobalTeardown->dump();
        mUnitTest->dump();
    }
    
    void accept(Visitor *v) {
        if(mGlobalMockup) mGlobalMockup->accept(v);
        if(mGlobalSetup) mGlobalSetup->accept(v);
        mUnitTest->accept(v);
        if(mGlobalTeardown)mGlobalTeardown->accept(v);
        v->VisitTestFile(this);
    }
};

class BufferAlloc : public TestExpr {
private:
    string StringRepresentation;
    string BufferSize;
    string DefaultValue;
public:

    BufferAlloc(const string& str) : StringRepresentation(str) , 
            BufferSize("1"), DefaultValue("0"){
        if(StringRepresentation[0] != '[' or StringRepresentation.back() != ']')
            throw Exception("Malformed buffer allocation");
        
        size_t pos = StringRepresentation.find(":");
        if(pos == string::npos) {
            pos = StringRepresentation.find("]");
            BufferSize = StringRepresentation.substr(1,pos-1);
        } else {
            BufferSize = StringRepresentation.substr(1,pos-1);
            size_t pos2 = StringRepresentation.find("]");
            DefaultValue = StringRepresentation.substr(pos+1,pos2-pos-1);
        }
    }

    ~BufferAlloc() { }
    
    
    string getBufferSize() const { return BufferSize; }
    string getDefaultValue() const { return DefaultValue; }
    
    void dump() {
        cout << StringRepresentation;
    }
    
    void accept(Visitor *v) {
        v->VisitBufferAlloc(this); // Doesn't have any children
    }
};

class FunctionArgument : public TestExpr{
protected:
    Argument *argArgument;
    BufferAlloc *argBuffAlloc;
    unsigned    ArgIndx;
    FunctionCallExpr *Parent;// Pointer to its parent
public:
    explicit FunctionArgument(Argument *arg) : 
        argArgument(arg), argBuffAlloc(nullptr), ArgIndx(0), Parent(nullptr) { }
    explicit FunctionArgument(BufferAlloc *arg) : 
        argArgument(nullptr), argBuffAlloc(arg), ArgIndx(0), Parent(nullptr) { }
    ~FunctionArgument() {}
    
    Tokenizer::Token getTokenType() const { 
        if(argArgument) return argArgument->getTokenType();
        return Tokenizer::TOK_BUFF_ALLOC;
    }
    unsigned getIndex() const { return ArgIndx; }
    FunctionCallExpr* getParent() const { return Parent; }
    void setIndex(unsigned ndx) {ArgIndx = ndx; }
    void setParent(FunctionCallExpr *ptr) {Parent = ptr; }
    const string& getStringRepresentation() const { 
        return argArgument->getStringRepresentation();
    }
    
    BufferAlloc* getBufferAlloc() const { return argBuffAlloc; }
    
    void dump() {
        cout<<Parent->getIdentifier()->getIdentifierStr()<<"(";
        cout<<ArgIndx<<":";
        if(argArgument)
            argArgument->dump();
        if(argBuffAlloc)
            argBuffAlloc->dump();
        cout<<")";
    }
    
    void accept(Visitor *v) {
        if(argArgument)
            argArgument->accept(v);
        if(argBuffAlloc)
            argBuffAlloc->accept(v);
        
        v->VisitFunctionArgument(this);// TODO IMplement
    }
};

class TestDriver {
private:
    Tokenizer mTokenizer;
    int mCurrentToken;
public:

    TestDriver() = delete;
    TestDriver(const TestDriver&) = delete;

    TestDriver(const string& file) : mTokenizer(file) { }

    ~TestDriver() {
    }

    TestExpr* ParseTestExpr();

private:
    Identifier* ParseIdentifier();
    Argument* ParseArgument();
    VariableAssignmentExpr* ParseVariableAssignment();
    BufferAlloc* ParseBufferAlloc();
    FunctionArgument* ParseFunctionArgument();
    Identifier* ParseFunctionName(); // We may want to have a FunctionName class
    FunctionCallExpr* ParseFunctionCall();
    ExpectedResult* ParseExpectedResult();
    ComparisonOperator* ParseComparisonOperator();
    TestTeardownExpr* ParseTestTearDown();
    TestFunction* ParseTestFunction();
    TestSetupExpr* ParseTestSetup();
    TestFixtureExpr* ParseTestFixture();
    MockupVariableExpr* ParseMockupVariable();
    MockupFunctionExpr* ParseMockupFunction();
    MockupFixtureExpr* ParseMockupFixture();
    TestMockupExpr* ParseTestMockup();
    TestDefinitionExpr* ParseTestDefinition();
    UnitTestExpr* ParseUnitTestExpr();
    GlobalMockupExpr* ParseGlobalMockupExpr();
    GlobalSetupExpr* ParseGlobalSetupExpr();
    GlobalTeardownExpr* ParseGlobalTeardownExpr();
    TestFile* ParseTestFile();
};

} // namespace tp
#endif	/* TESTPARSER_HXX */

