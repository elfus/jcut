/*
 * File:   TestParser.h
 * Author: Adrián Ortega García
 *
 * Created on April 17, 2014, 10:09 AM
 */

#ifndef TESTPARSER_H
#define	TESTPARSER_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include "Visitor.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Function.h"

using namespace std;


class LLVMFunctionHolder {
private:
    llvm::Function* mFunction;
    llvm::GenericValue mReturnValue;
    /// Attribute used to know the group this LLVM Function belongs to
    string  mGroupName;
public:
    LLVMFunctionHolder() : mFunction(nullptr) {}
    virtual ~LLVMFunctionHolder() {delete mFunction;}

    void setLLVMFunction(llvm::Function* f) { mFunction = f; }
    llvm::Function* getLLVMFunction() const { return mFunction; }
    void setReturnValue(llvm::GenericValue GV) { mReturnValue = GV; }
    llvm::GenericValue getReturnValue () const { return mReturnValue; }
    string getGroupName() const { return mGroupName; }
    void setGroupName(const string& name) { mGroupName = name; }
};

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
        TOK_TEST_INFO = -106,
        TOK_GROUP = -107,
    };

    enum Identifier {
        ID_UNKNOWN = 0,
        ID_FUNCTION,
        ID_VARIABLE,
        ID_CONSTANT
    };

    Tokenizer(const string& filename);

    ~Tokenizer() {}

    Token currentToken() {
        return mCurrentToken;
    }

    int nextToken();
    int peekToken();

    int getInteger() { return mInt; }

    float getDouble() { return mDouble; }

    string getBuffAlloc() { return mBuffAlloc;  }

    string getTokenStringValue() { return mTokStrValue; }

    char getChar() { return mLastChar; }

    Identifier getIdentifierType() {  return mIdType;  }

    bool isCharIgnored(char);

private:
    std::ifstream mInput;
    Token mCurrentToken;
    string mFunction;
    char mEqOp;
    int mInt;
    float mDouble;
    string mBuffAlloc;
    string mTokStrValue;
    char mLastChar;
    Identifier mIdType;
    unsigned mOldPos;
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

    Type getType() const { return mType; }
    string getTypeStr() const { return mStringRepresentation; }
private:
    string mStringRepresentation;
    Type mType;
};

class NumericConstant : public TestExpr{
private:
    union {
        int ic;
        float fc;
    } mNC;
    bool mIsInt;
public:
    explicit NumericConstant(int i) : mIsInt(true) {
        mNC.ic = i;
    }
    explicit NumericConstant(float f) : mIsInt(false) {
        mNC.fc = f;
    }
    ~NumericConstant() {}

    void dump() {
        cout << "int = " << mNC.ic << ", float = " << mNC.fc;
    }
    void accept(Visitor* v) {
        v->VisitNumericConstant(this);
    }

    int getInt() const { return mNC.ic; }
    bool isInt() const { return mIsInt; }
    bool isFloat() const { return !mIsInt; }
};

class StringConstant : public TestExpr{
private:
    string mStr;
public:
    StringConstant(const string& str) : mStr(str) {}

    ~StringConstant() {}

    void dump() {
        cout << mStr;
    }
    void accept(Visitor* v) {
        v->VisitStringConstant(this);
    }
};

class CharConstant : public TestExpr{
private:
    char mC;
public:
    explicit CharConstant(char C) : mC(C) {}
    ~CharConstant() {}
    void dump() {
        cout << "char = " << mC;
    }
    void accept(Visitor* v) {
        v->VisitCharConstant(this);
    }
    int getChar() const { return mC; }
};

class Constant : public TestExpr {
public:
    enum Type {
        NUMERIC,
        STRING,
        CHAR,
        INVALID,
    };
private:
    NumericConstant* mNC;
    StringConstant* mSC;
    CharConstant* mCC;
    Type    mType;
public:
    Constant(NumericConstant *C) : mNC(C), mSC(nullptr), mCC(nullptr), mType(NUMERIC) {}
    Constant(StringConstant *C) : mNC(nullptr), mSC(C), mCC(nullptr), mType(STRING) {}
    Constant(CharConstant *C) : mNC(nullptr), mSC(nullptr), mCC(C), mType(CHAR) {}
    ~Constant() {
        if(mNC) delete mNC;
        if(mSC) delete mSC;
        if(mCC) delete mCC;
    }

    void dump() {
        if(mNC) mNC->dump();
        if(mSC) mSC->dump();
        if(mCC) mCC->dump();
    }
    void accept(Visitor* v) {
        if(mNC) mNC->accept(v);
        if(mSC) mSC->accept(v);
        if(mCC) mCC->accept(v);
        v->VisitConstant(this);
    }

    int getValue() const {
        if(mNC) return mNC->getInt();
        // @bug String not supported yet
        if(mCC) return (int) mCC->getChar();
        return 0;
    }

    Type getType() const { return mType; }
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

class Operand : public TestExpr {
private:
    Constant* mC;
    Identifier* mI;
public:
    explicit Operand(Constant* C) : mC(C), mI(nullptr) {}
    explicit Operand(Identifier* I) : mC(nullptr), mI(I) {}
    ~Operand() {
        if (mC)
            delete mC;
        if(mI)
            delete mI;
    }

    void dump() {
        if(mC)
            mC->dump();
        if(mI)
            mI->dump();
    }

    void accept(Visitor* v) {
        if(mC)
            mC->accept(v);
        if(mI)
            mI->accept(v);
        v->VisitOperand(this);
    }

    bool isConstant() const { return (mC) ? true : false; }
    bool isIdentifier() const { return (mI) ? true : false; }
    Identifier* getIdentifier() const { return mI;}
    Constant* getConstant() const { return mC; }
};

class ExpectedConstant : public TestExpr {
private:
    Constant* mC;
public:
    ExpectedConstant() = delete;
    explicit ExpectedConstant(Constant *C) : mC(C) {}
    ~ExpectedConstant() { delete mC;}

    void dump() {
        mC->dump();
    }
    void accept(Visitor* v) {
        mC->accept(v);
        v->VisitExpectedConstant(this);
    }

    int getValue() const {
        return mC->getValue();
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

    const string& getStringRepresentation() const { return StringRepresentation; }
    Tokenizer::Token getTokenType() const { return TokenType; }

    void dump() {
        cout << StringRepresentation;
    }

    void accept(Visitor *v) {
       v->VisitArgument(this); // Argument doesn't have any children
    }
};

class FunctionArgument;

class FunctionCall : public TestExpr {
private:
    Identifier *FunctionName;
    vector<FunctionArgument*> FunctionArguments;
public:

    Identifier *getIdentifier() const {return FunctionName;}

    FunctionCall(Identifier* name, const vector<FunctionArgument*>& arg);

    virtual ~FunctionCall();

    void dump();

    void accept(Visitor *v);
};

class ExpectedResult : public TestExpr {
private:
    ComparisonOperator* mCompOp;
    ExpectedConstant* mEC;
public:
    ExpectedResult(ComparisonOperator* cmp, ExpectedConstant* Arg) :
    mCompOp(cmp), mEC(Arg) {}
    ~ExpectedResult() {
        delete mEC;
        delete mCompOp;
    }
    void dump() {
        mCompOp->dump();
        mEC->dump();
    }

    void accept(Visitor *v) {
        mCompOp->accept(v);
        mEC->accept(v);
        v->VisitExpectedResult(this);
    }

    ComparisonOperator* getComparisonOperator() const { return mCompOp; }
    ExpectedConstant* getExpectedConstant() const { return mEC; }
};

class ExpectedExpression : public TestExpr {
private:
    Operand* mLHS;
    ComparisonOperator* mCO;
    Operand* mRHS;
public:
    ExpectedExpression(Operand* LHS, ComparisonOperator* CO, Operand* RHS) :
    mLHS(LHS), mCO(CO), mRHS(RHS) {  }
    ~ExpectedExpression() {
        delete mLHS;
        delete mCO;
        delete mRHS;
    }

    void dump() {
        mLHS->dump();
        mCO->dump();
        mRHS->dump();
    }

    void accept(Visitor* v) {
        mLHS->accept(v);
        mCO->accept(v);
        mRHS->accept(v);
        v->VisitExpectedExpression(this);
    }

    Operand* getLHSOperand() const { return mLHS; }
    Operand* getRHSOperand() const { return mRHS; }
    ComparisonOperator* getComparisonOperator() const { return mCO; }
};

class VariableAssignment : public TestExpr {
private:
    Identifier *mIdentifier;
    Argument *mArgument;
public:

    VariableAssignment(Identifier *id, Argument *arg) :
    mIdentifier(nullptr), mArgument(nullptr) {
        mIdentifier = dynamic_cast<Identifier*> (id);
        if (mIdentifier == nullptr)
            throw Exception("Invalid Identifier type");

        mArgument = dynamic_cast<Argument*> (arg);
        if (mArgument == nullptr)
            throw Exception("Invalid Argument type");
    }

    ~VariableAssignment() {
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
        v->VisitVariableAssignment(this);
    }
};

class MockupVariable : public TestExpr {
private:
    VariableAssignment *mVariableAssignment;
public:

    MockupVariable(VariableAssignment *var) : mVariableAssignment(nullptr) {
        mVariableAssignment = dynamic_cast<VariableAssignment*> (var);
        if (mVariableAssignment == nullptr)
            throw Exception("Invalid Variable Assignment Expression type");
    }

    ~MockupVariable() {
        if (mVariableAssignment)
            delete mVariableAssignment;
    }

    void dump() {
        mVariableAssignment->dump();
    }

    void accept(Visitor *v) {
        mVariableAssignment->accept(v);
        v->VisitMockupVariable(this);
    }
};

class MockupFunction : public TestExpr {
private:
    FunctionCall *mFunctionCall;
    Argument *mArgument;
public:

    MockupFunction(FunctionCall *call, Argument *arg) :
    mFunctionCall(nullptr), mArgument(nullptr) {
        mFunctionCall = dynamic_cast<FunctionCall*> (call);
        if (mFunctionCall == nullptr)
            throw Exception("Invalid FunctionCallExpr type");

        mArgument = dynamic_cast<Argument*> (arg);
        if (mArgument == nullptr)
            throw Exception("Invalid Argument type");
    }

    ~MockupFunction() {
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
        v->VisitMockupFunction(this);
    }
};

class MockupFixture : public TestExpr {
private:
    vector<MockupFunction*> mMockupFunctions;
    vector<MockupVariable*> mMockupVariables;
public:
    MockupFixture(const vector<MockupFunction*>& func,
            const vector<MockupVariable*>& var) :
    mMockupFunctions(func), mMockupVariables(var) {
    }

    ~MockupFixture() {
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

        v->VisitMockupFixture(this);
    }
};

class TestMockup : public TestExpr {
private:
    MockupFixture *mMockupFixture;
public:

    TestMockup(MockupFixture *fixt) : mMockupFixture(fixt) {
    }

    ~TestMockup() {
        if (mMockupFixture) delete mMockupFixture;
    }

    void dump() {
        cout << "mockup {" << endl;
        mMockupFixture->dump();
        cout << "}" << endl;
    }

    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitTestMockup(this);
    }
};

class TestFixture : public TestExpr {
private:
    vector<FunctionCall*> mFunctionCalls;
    vector<VariableAssignment*> mVarAssign;
    vector<ExpectedExpression*> mExp;
public:

    TestFixture(const vector<FunctionCall*>& func,
            const vector<VariableAssignment*>& var,
            const vector<ExpectedExpression*>& exp) :
    mFunctionCalls(func), mVarAssign(var), mExp(exp) {

    }

    ~TestFixture() {
        for (auto*& ptr : mFunctionCalls)
            delete ptr;

        for (auto*& ptr : mVarAssign)
            delete ptr;

        for (auto*& ptr : mExp)
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

        if (mExp.size()) {
            cout << endl << "EXPECTED EXPRESSIONS" << endl;
            for (auto*& ptr : mExp) {
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

        for (auto*& ptr : mExp)
            ptr->accept(v);

        v->VisitTestFixture(this);
    }
};

class TestSetup : public TestExpr {
private:
    TestFixture* mTestFixtureExpr;
public:

    TestSetup(TestFixture* fixture) : mTestFixtureExpr(fixture) {
    }

    ~TestSetup() {
        if (mTestFixtureExpr) delete mTestFixtureExpr;
    }

    void dump() {
        cout << "before {";
        mTestFixtureExpr->dump();
        cout << "}" << endl;
    }

    void accept(Visitor *v) {
        mTestFixtureExpr->accept(v);
        v->VisitTestSetup(this);
    }
};

class TestFunction : public TestExpr {
private:
    FunctionCall* mFunctionCall;
    ExpectedResult* mExpectedResult;
public:
    TestFunction(FunctionCall *F, ExpectedResult* E=nullptr) : mFunctionCall(F),
            mExpectedResult(E) {}
    ~TestFunction() {
        if(mFunctionCall) delete mFunctionCall;
        if(mExpectedResult) delete mExpectedResult;
    }

    FunctionCall* getFunctionCall() const {return mFunctionCall; }
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

class TestTeardown : public TestExpr {
private:
    TestFixture* mTestFixture;
public:

    TestTeardown(TestFixture* fixture) : mTestFixture(fixture) {
    }

    ~TestTeardown() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "after {";
        mTestFixture->dump();
        cout << "}" << endl << endl;
    }

    void accept(Visitor *v) {
        mTestFixture->accept(v);
        v->VisitTestTeardow(this);
    }
};

class TestInfo : public TestExpr {
private:
    Identifier* mTestName;
public:
    TestInfo(Identifier* name) : mTestName(name) {}
    ~TestInfo() { delete mTestName; }

    void dump() {

    }

    void accept(Visitor* v) {
        v->VisitTestInfo(this);
    }
};

class TestDefinition : public TestExpr, public LLVMFunctionHolder {
private:
    TestInfo *mTestInfo;
    TestFunction *FunctionCall;
    TestSetup *mTestSetup;
    TestTeardown *mTestTeardown;
    TestMockup *mTestMockup;
    string mTestName;

public:

    TestDefinition(
            TestInfo *info,
            TestFunction *function,
            TestSetup *setup = nullptr,
            TestTeardown *teardown = nullptr,
            TestMockup *mockup = nullptr) :
    mTestInfo(info), FunctionCall(function), mTestSetup(setup),
    mTestTeardown(teardown), mTestMockup(mockup), mTestName()  {
    }

    TestFunction * getTestFunction() const {return FunctionCall;}

    virtual ~TestDefinition() {
        if (mTestInfo) delete mTestInfo;
        if (FunctionCall) delete FunctionCall;
        if (mTestSetup) delete mTestSetup;
        if (mTestTeardown) delete mTestTeardown;
        if (mTestMockup) delete mTestMockup;
    }

    void dump() {
        if (mTestMockup) {
            mTestMockup->dump();
        }
        if (mTestSetup) {
            mTestSetup->dump();
        }
        FunctionCall->dump();
        cout << endl;
        if (mTestTeardown) {
            mTestTeardown->dump();
        }
    }

    void accept(Visitor *v) {
        if(mTestInfo) mTestInfo->accept(v);
        if(mTestMockup) mTestMockup->accept(v);
        if(mTestSetup) mTestSetup->accept(v);
        FunctionCall->accept(v);
        if(mTestTeardown) mTestTeardown->accept(v);
        v->VisitTestDefinition(this);
    }

    void setTestName(const string& name) { mTestName = name; }
    string getTestName() const { return mTestName; }
};

class GlobalMockup : public TestExpr {
private:
    MockupFixture *mMockupFixture;
public:

    GlobalMockup(MockupFixture *fixt) : mMockupFixture(fixt) {    }

    ~GlobalMockup() {
        if (mMockupFixture) delete mMockupFixture;
    }

    void dump() {
        cout << "mockup_all {" << endl;
        mMockupFixture->dump();
        cout << "}" << endl << endl;
    }

    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitGlobalMockup(this);
    }
};

class GlobalSetup : public TestExpr, public LLVMFunctionHolder {
private:
    TestFixture *mTestFixture;
public:

    GlobalSetup(TestFixture *fixt) : mTestFixture(fixt) {    }

    ~GlobalSetup() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "before_all {";
        mTestFixture->dump();
        cout << "}" << endl << endl;
    }

    void accept(Visitor *v) {
        v->VisitGlobalSetupFirst(this);
        mTestFixture->accept(v);
        v->VisitGlobalSetup(this);
    }
};

class GlobalTeardown : public TestExpr, public LLVMFunctionHolder {
private:
    TestFixture *mTestFixture;
public:

    GlobalTeardown() : mTestFixture(nullptr) {
    }

    GlobalTeardown(TestFixture *fixt) : mTestFixture(fixt) {
    }

    ~GlobalTeardown() {
        if (mTestFixture) delete mTestFixture;
    }

    void dump() {
        cout << "after_all {" << endl;
        mTestFixture->dump();
        cout << "}" << endl;
    }

    void accept(Visitor *v) {
        v->VisitGlobalTeardownFirst(this);
        // This a workaround because we can build a GlobalTeardown with or without
        // a TestFixture
        if (mTestFixture) mTestFixture->accept(v);
        v->VisitGlobalTeardown(this);
    }
};

class TestGroup : public TestExpr {
private:
    Identifier* mName;
    // Let's work with tests as we find them, do not alter their order.
    vector<TestExpr*> mTests;
    GlobalMockup *mGlobalMockup;
    GlobalSetup *mGlobalSetup;
    GlobalTeardown *mGlobalTeardown;
    static int group_count;
public:
    TestGroup(Identifier* name,
            const vector<TestExpr*>& tests,
            GlobalMockup *gm = nullptr,
            GlobalSetup *gs = nullptr, GlobalTeardown *gt = nullptr) :
    mName(name), mTests(tests),
    mGlobalMockup(gm), mGlobalSetup(gs), mGlobalTeardown(gt) {
        if (!mName) {
            string group_name = "group_";
            group_name += ((char) (((int) '0') + (group_count++)));
            mName = new Identifier(group_name);
        }
        cout<<"Group name: "<<mName->getIdentifierStr()<<endl;
        /// @todo Enable GlobalMockup
        // if (GlobalMockup) GlobalMockup->setGroupName(mName->getIdentifierStr());
        if (mGlobalSetup) mGlobalSetup->setGroupName(mName->getIdentifierStr());
        if (mGlobalTeardown) mGlobalTeardown->setGroupName(mName->getIdentifierStr());
    }
    ~TestGroup() {
        if (mGlobalMockup) delete mGlobalMockup;
        if (mGlobalSetup) delete mGlobalSetup;
        if (mGlobalTeardown) delete mGlobalTeardown;
        for (auto*& ptr : mTests)
            delete ptr;
    }

    void dump() {
        if (mGlobalMockup) mGlobalMockup->dump();
        if (mGlobalSetup) mGlobalSetup->dump();
        if (mGlobalTeardown) mGlobalTeardown->dump();
        for (auto*& ptr : mTests) {
            ptr->dump();
            cout << endl;
        }
    }

    void accept(Visitor *v) {
        v->VisitTestGroupFirst(this);
        if (mGlobalMockup) mGlobalMockup->accept(v);
        if (mGlobalSetup) mGlobalSetup->accept(v);
        for (auto*& ptr : mTests) {
            ptr->accept(v);
        }
        if (mGlobalTeardown) mGlobalTeardown->accept(v);
        v->VisitTestGroup(this);
    }

    string getGroupName() const { return mName->getIdentifierStr(); }
    GlobalSetup* getGlobalSetup() const { return mGlobalSetup; }
    GlobalTeardown* getGlobalTeardown() const { return mGlobalTeardown; }
    void setGlobalTeardown(GlobalTeardown* gt) { mGlobalTeardown = gt; }
};

class UnitTests : public TestExpr {
private:
    TestGroup* mTestGroups;
public:

    UnitTests(TestGroup* groups) : mTestGroups(groups) { }

    virtual ~UnitTests() {
        delete mTestGroups;
    }

    void dump() {
        mTestGroups->dump();
    }

    void accept(Visitor *v) {
        v->VisitUnitTestFirst(this);
        mTestGroups->accept(v);
        v->VisitUnitTest(this);
    }
};

class TestFile : public TestExpr {
private:
    UnitTests *mUnitTest;
public:

    TestFile(UnitTests *ut) :
    mUnitTest(ut) {

    }

    ~TestFile() {
        if (mUnitTest) delete mUnitTest;
    }

    void dump() {
        mUnitTest->dump();
    }

    void accept(Visitor *v) {
        mUnitTest->accept(v);
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
    FunctionCall *Parent;// Pointer to its parent
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
    FunctionCall* getParent() const { return Parent; }
    void setIndex(unsigned ndx) {ArgIndx = ndx; }
    void setParent(FunctionCall *ptr) {Parent = ptr; }
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
    VariableAssignment* ParseVariableAssignment();
    BufferAlloc* ParseBufferAlloc();
    FunctionArgument* ParseFunctionArgument();
    Identifier* ParseFunctionName(); // We may want to have a FunctionName class
    FunctionCall* ParseFunctionCall();
    ExpectedResult* ParseExpectedResult();
    ComparisonOperator* ParseComparisonOperator();
    ExpectedConstant* ParseExpectedConstant();
    ExpectedExpression* ParseExpectedExpression();
    Operand* ParseOperand();
    Constant* ParseConstant();
    TestTeardown* ParseTestTearDown();
    TestFunction* ParseTestFunction();
    TestSetup* ParseTestSetup();
    TestFixture* ParseTestFixture();
    MockupVariable* ParseMockupVariable();
    MockupFunction* ParseMockupFunction();
    MockupFixture* ParseMockupFixture();
    TestMockup* ParseTestMockup();
    TestInfo* ParseTestInfo();
    TestDefinition* ParseTestDefinition();
    // @arg name The name of the group to be parsed
    TestGroup* ParseTestGroup(Identifier* name = nullptr);
    UnitTests* ParseUnitTest();
    GlobalMockup* ParseGlobalMockup();
    GlobalSetup* ParseGlobalSetup();
    GlobalTeardown* ParseGlobalTeardown();
    TestFile* ParseTestFile();
};

} // namespace tp
#endif	/* TESTPARSER_H */
