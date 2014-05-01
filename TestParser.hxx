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

class Tokenizer {
public:
    enum Token {
        TOK_ERR = -1,
        TOK_EOF = -2,
        TOK_ASCII_CHAR = -3,
        TOK_IDENTIFIER = -4,
        TOK_EQ_OP = -5,
        TOK_INT = -6,
        TOK_DOUBLE = -7, // Will handle floats too
        TOK_CHAR = -8,
        TOK_STRING = -9,
        TOK_BUFF_ALLOC = -10,
        TOK_ARRAY_INIT = -11,
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

class UnitTestExpr : public TestExpr {
private:
    vector<TestExpr*> TestDefinitions;
public:

    UnitTestExpr(const vector<TestExpr*>& def) : TestDefinitions(def) { }

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
        if(v->VisitUnitTestExpr(this))
            for (auto*& ptr : TestDefinitions) {
                ptr->accept(v);
            }
    }
};

class Argument : public TestExpr {
protected:
    string StringRepresentation;
    Tokenizer::Token TokenType;
public:

    Argument(const string& str, Tokenizer::Token token) :
    StringRepresentation(str), TokenType(token) { }

    ~Argument() {}

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

class FunctionCallExpr : public TestExpr {
private:
    Identifier *FunctionName;
    vector<TestExpr*> FunctionArguments;
public:
    
    Identifier *getIdentifier() const {return FunctionName;}

    FunctionCallExpr(Identifier* name, const vector<TestExpr*>& arg) :
    FunctionName(name), FunctionArguments(arg) {
    }

    virtual ~FunctionCallExpr() {
        delete FunctionName;
        for (TestExpr*& ptr : FunctionArguments) {
            delete ptr;
            ptr = nullptr;
        }
    }

    void dump() {
        FunctionName->dump();
        cout << "(";
        for (auto arg : FunctionArguments) {
            arg->dump();
            cout << ",";
        }
        cout << ")";
    }
    
    void accept(Visitor *v) {
        if(v->VisitFunctionCallExpr(this)) {
            for (TestExpr*& ptr : FunctionArguments) {
                ptr->accept(v);
            }
        }
    }
};

class TestDefinitionExpr : public TestExpr {
private:
    FunctionCallExpr *FunctionCall;
    TestExpr *ExpectedResult, *TestSetup, *TestTeardown, *TestMockup;
    
public:

    TestDefinitionExpr(FunctionCallExpr *function, TestExpr *expected = nullptr,
            TestExpr *setup = nullptr, TestExpr *teardown = nullptr,
            TestExpr *mockup = nullptr) :
    FunctionCall(function), ExpectedResult(expected), TestSetup(setup),
    TestTeardown(teardown), TestMockup(mockup) {
    }
    
    FunctionCallExpr* getFunctionCall() const {return FunctionCall;}

    virtual ~TestDefinitionExpr() {
        if (FunctionCall) delete FunctionCall;
        if (ExpectedResult) delete ExpectedResult;
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
        if (ExpectedResult)
            ExpectedResult->dump();
        cout << endl;
        if (TestTeardown) {
            TestTeardown->dump();
        }
    }
    
    void accept(Visitor *v) {
        if(v->VisitTestDefinitionExpr(this)) {
            if(TestMockup) TestMockup->accept(v);
            if(TestSetup) TestSetup->accept(v);
            FunctionCall->accept(v);
            if(ExpectedResult) ExpectedResult->accept(v);
            if(TestTeardown) TestTeardown->accept(v);
        }
    }
};


class VariableAssignmentExpr : public TestExpr {
private:
    Identifier *mIdentifier;
    Argument *mArgument;
public:

    VariableAssignmentExpr(TestExpr *id, TestExpr *arg) :
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

    void dump() {
        mIdentifier->dump();
        cout << " = ";
        mArgument->dump();
    }
    
    void accept(Visitor *v){
        if(v->VisitVariableAssignmentExpr(this)) {
            mIdentifier->accept(v);
            mArgument->accept(v);
        }
    }
};

class MockupVariableExpr : public TestExpr {
private:
    VariableAssignmentExpr *mVariableAssignment;
public:

    MockupVariableExpr(TestExpr *var) : mVariableAssignment(nullptr) {
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
        if(v->VisitMockupVariableExpr(this))
                mVariableAssignment->accept(v);
    }
};

class MockupFunctionExpr : public TestExpr {
private:
    FunctionCallExpr *mFunctionCall;
    Argument *mArgument;
public:

    MockupFunctionExpr(TestExpr *call, TestExpr *arg) :
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
        if(v->VisitMockupFunctionExpr(this))
                mFunctionCall->accept(v);
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
        if(v->VisitMockupFixtureExpr(this)) {
            for (auto*& ptr : mMockupVariables)
                ptr->accept(v);

            for (auto*& ptr : mMockupFunctions)
                ptr->accept(v);
        }
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
        if(v->VisitTestMockupExpr(this))
                mMockupFixture->accept(v);
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
        if(v->VisitTestFixtureExpr(this)) {
            for (auto*& ptr : mFunctionCalls)
                ptr->accept(v);

            for (auto*& ptr : mVarAssign)
                ptr->accept(v);
        }
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
        // TODO v->VisitTestSetupExpr(this)
    }
};

class TestTeardowExpr : public TestExpr {
private:
    TestFixtureExpr* mTestFixture;
public:

    TestTeardowExpr(TestFixtureExpr* fixture) : mTestFixture(fixture) {
    }

    ~TestTeardowExpr() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "after {";
        mTestFixture->dump();
        cout << "}" << endl << endl;
    }
    
    void accept(Visitor *v) {
        if(v->VisitTestTeardowExpr(this))
            mTestFixture->accept(v);
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
        if(v->VisitGlobalMockupExpr(this))
            mMockupFixture->accept(v);
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
        if(v->VisitGlobalSetupExpr(this))
            mTestFixture->accept(v);
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
        if(v->VisitGlobalTeardownExpr(this))
            mTestFixture->accept(v);
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
        if(v->VisitTestFile(this)) {
            if(mGlobalMockup) mGlobalMockup->accept(v);
            if(mGlobalSetup) mGlobalSetup->accept(v);
            mUnitTest->accept(v);
            if(mGlobalTeardown)mGlobalTeardown->accept(v);
        }
    }
};

class BufferAlloc : public TestExpr {
private:
    string StringRepresentation;
public:

    BufferAlloc(const string& str) : StringRepresentation(str) { }

    ~BufferAlloc() { }

    void dump() {
        cout << StringRepresentation;
    }
    
    void accept(Visitor *v) {
        v->VisitBufferAlloc(this); // Doesn't have any children
    }
};

class TestParser {
private:
    Tokenizer mTokenizer;
    int mCurrentToken;
public:

    TestParser() = delete;
    TestParser(const TestParser&) = delete;

    TestParser(const string& file) : mTokenizer(file) { }

    ~TestParser() {
    }

    TestExpr* ParseTestExpr();

private:
    Identifier* ParseIdentifier();
    Argument* ParseArgument();
    VariableAssignmentExpr* ParseVariableAssignment();
    BufferAlloc* ParseBufferAlloc();
    TestExpr* ParseFunctionArgument();
    Identifier* ParseFunctionName(); // We may want to have a FunctionName class
    FunctionCallExpr* ParseFunctionCall();
    TestTeardowExpr* ParseTestTearDown();
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


#endif	/* TESTPARSER_HXX */

