//===-- jcut/TestParser.h - Grammar Rules -----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
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

#ifndef TESTPARSER_H
#define	TESTPARSER_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include "JCUTScanner.h"

#include "Visitor.h"

#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"

using namespace std;

class Exception : public std::exception {
public:
    enum Severity {
        ERROR = 0,
        WARNING
    };
    // @todo Redesign the Exception class to take a Token in the constructor.

    Exception(const string& expected, const string& received)
        : mMsg(""), mExtraMsg(""), mLine(0), mColumn(0), mSeverity(ERROR) {
        mMsg = "Expected "+expected+" but received: "+received;
    }

    /// Used for warnings
    Exception(const string& msg,
               const string& extra = "", Severity s=Severity::ERROR)
        : mMsg(msg), mExtraMsg(extra), mLine(0), mColumn(0), mSeverity(s) {
        init();
    }

    /// Used for errors
    Exception(unsigned line, unsigned column, const string& msg,
               const string& extra = "", Severity s=Severity::ERROR)
        : mMsg(msg), mExtraMsg(extra), mLine(line), mColumn(column),mSeverity(s) {
        init();
    }

    const char* what() const throw () {
        return mMsg.c_str();
    }

    static string mCurrentFile;
private:
    string mMsg;
    string mExtraMsg;
    unsigned mLine;
    unsigned mColumn;
    Severity mSeverity;

    void init() {
        stringstream ss;
        if(mSeverity == Severity::ERROR)
            ss << mCurrentFile <<":"<<mLine<<":"<<mColumn<<": error: " << mMsg;
        else if(mSeverity == Severity::WARNING)
            ss << "warning" << ": " << mMsg;
        if (mExtraMsg.size()) {
            ss << endl << "\t" << mExtraMsg;
        }
        mMsg = ss.str();
    }
};

class LLVMFunctionHolder {
private:
    // Seems LLVM Is deleting this function. You may want to investigate.
    llvm::Function* mFunction;
    llvm::GenericValue mReturnValue;
    /// Attribute used to know the group this LLVM Function belongs to
    string  mGroupName;
    // The output when running this function.
    string  mOutput;
    // Any warning we want to inform while generating this LLVM Function
    vector<Exception> mWarnings;
    bool mPassingValue;
    llvm::GlobalVariable* mResultVariable;
public:
    LLVMFunctionHolder() : mFunction(nullptr), mReturnValue(), mGroupName(),
    mOutput(), mWarnings(), mPassingValue(false), mResultVariable(nullptr) {}
    virtual ~LLVMFunctionHolder() { }

    void setLLVMFunction(llvm::Function* f) { mFunction = f; }
    llvm::Function* getLLVMFunction() const { return mFunction; }
    void setReturnValue(llvm::GenericValue GV) { mReturnValue = GV; }
    const llvm::GenericValue& getReturnValue () const { return mReturnValue; }
    string getGroupName() const { return mGroupName; }
    void setGroupName(const string& name) { mGroupName = name; }
    void setOutput(string&& output) { mOutput = output; }
    const string& getOutput() const { return mOutput; }
    void setWarnings(const vector<Exception>& warnings) {
        mWarnings = warnings;
        warning_count += mWarnings.size();
    }
    const vector<Exception>& getWarnings() const { return mWarnings; }
    void setPassingValue(bool passed) { mPassingValue = passed; }
    bool getPassingValue() const { return mPassingValue; }
    void setGlobalVariable(llvm::GlobalVariable* g) { mResultVariable = g; }
    llvm::GlobalVariable* getGlobalVariable() const { return mResultVariable; }
    static unsigned warning_count;
};

namespace tp { // tp stands for test parser

class Token {
public:
    Token() : mType(TOK_ERR), mLine(0), mColumn(0), mLexeme("") {}
    Token(char* lex, int size, int type, unsigned line, unsigned column) :
    mType(static_cast<TokenType>(type)), mLine(line), mColumn(column), mLexeme(lex,size) {}

    bool operator ==(char c) {
        string s(1,c);
        return mLexeme == s;
    }
    bool operator !=(char c) {
        string s(1,c);
        return mLexeme != s;
    }
    bool operator ==(TokenType type) { return mType == type; }
    bool operator !=(TokenType type) { return mType != type; }

    TokenType    mType;
    unsigned mLine, mColumn;
    string  mLexeme;
};

ostream& operator << (ostream& os, Token& token);

class Tokenizer {
public:
	Tokenizer() : mTokens(), mNextToken(nullptr) {}

    ~Tokenizer() {}

    void tokenize(const string& filename);
    void tokenize(const char* buffer);

    Token nextToken();
    Token peekToken();
private:
    vector<Token>   mTokens;
    vector<Token>::iterator mNextToken;
};

class TestExpr {
public:
	enum Type {
		OTHER = 0,
		FUNC_CALL,
		VAR_ASSIGN,
		EXPECT_EXPR
	};
    TestExpr() : line(0), column(0), type(OTHER) { ++leaks; }
    TestExpr(const TestExpr& that)
    : line(that.line), column(that.column), type(that.type) {}

    virtual void accept(Visitor *) = 0;

    virtual ~TestExpr() {--leaks;}

    static int leaks;

    // The line and column where this expression start.
    // Due to time constraints only a few subclasses will set these values.
    /// @todo Make sure all subclasses set these values to do a better
    /// error reporting.
    void setLine(int l) { line = l; }
    int getLine() const { return line; }
    void setColumn(int c) { column = c; }
    int getColumn() const { return column; }
    Type getType() const { return type; }
protected:
    int line;
    int column;
    Type type;
};

class DataPlaceholder : public TestExpr {
public:
	void accept(Visitor *v) {
		//@ note For the moment we don't need to VisitDataplaceholder
	}
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
    ComparisonOperator(const ComparisonOperator& that)
    : TestExpr(that),
      mStringRepresentation(that.mStringRepresentation), mType(that.mType) {}

    void accept(Visitor* v) {
        v->VisitComparisonOperator(this);
    }

    Type getType() const { return mType; }
    string toString() const { return mStringRepresentation; }
private:
    string mStringRepresentation;
    Type mType;
};

class NumericConstant : public TestExpr{
private:
    int mIC;
    float mFC;
    bool mIsInt;
public:
    explicit NumericConstant(int i) : mIC(i), mFC(0), mIsInt(true) { }
    explicit NumericConstant(float f) : mIC(0), mFC(f), mIsInt(false) { }
    NumericConstant(const NumericConstant& that)
    : TestExpr(that), mIC(that.mIC), mFC(that.mFC), mIsInt(that.mIsInt) {}
    ~NumericConstant() {}

    void accept(Visitor* v) {
        v->VisitNumericConstant(this);
    }

    int getInt() const { return mIC; }
    float getFloat() const { return mFC; }
    bool isInt() const { return mIsInt; }
    bool isFloat() const { return !mIsInt; }

    const string toString() const {
    	stringstream ss;
    	if(isInt())
    		ss << mIC;
    	else if(isFloat())
    		ss << mFC;
    	return ss.str();
    }
};

class StringConstant : public TestExpr{
private:
    string mStr;
public:
    StringConstant(const string& str) : mStr(str) {}
    StringConstant(const StringConstant& that) : TestExpr(that), mStr(that.mStr) {}

    ~StringConstant() {}

    void accept(Visitor* v) {
        v->VisitStringConstant(this);
    }

    const string& getString() const { return mStr; }
};

class CharConstant : public TestExpr{
private:
    char mC;
public:
    explicit CharConstant(char C) : mC(C) {}
    CharConstant(const CharConstant& that) : TestExpr(that), mC(that.mC) {}
    ~CharConstant() {}

    void accept(Visitor* v) {
        v->VisitCharConstant(this);
    }
    char getChar() const { return mC; }
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
    unique_ptr<NumericConstant> mNC;
    unique_ptr<StringConstant> mSC;
    unique_ptr<CharConstant> mCC;
    Type    mType;
    string mStr; // String representation of this constant regardless its type
public:
    Constant(NumericConstant *C) : mNC(C), mSC(nullptr), mCC(nullptr), mType(NUMERIC) {
    	stringstream ss;
    	if(C->isInt())
    		ss << C->getInt();
    	if(C->isFloat())
    		ss << C->getFloat();
    	mStr = ss.str();
    }
    Constant(StringConstant *C) : mNC(nullptr), mSC(C), mCC(nullptr), mType(STRING),
    		mStr(C->getString()){}
    Constant(CharConstant *C) : mNC(nullptr), mSC(nullptr), mCC(C), mType(CHAR) {
    		mStr = C->getChar();
    }
    Constant(const Constant& that)
    : TestExpr(that), mNC(nullptr), mSC(nullptr), mCC(nullptr), mType(INVALID) {
    	if(that.mNC) {
    		mNC = unique_ptr<NumericConstant>(new NumericConstant(*that.mNC));
    		stringstream ss;
    		if(mNC->isInt())
				ss << mNC->getInt();
			if(mNC->isFloat())
				ss << mNC->getFloat();
			mStr = ss.str();
    	}
    	if(that.mSC) mSC = unique_ptr<StringConstant>(new StringConstant(*that.mSC));
    	if(that.mCC) mCC = unique_ptr<CharConstant>(new CharConstant(*that.mCC));
    	mType = that.mType;
    }

    void accept(Visitor* v) {
        if(mNC) mNC->accept(v);
        if(mSC) mSC->accept(v);
        if(mCC) mCC->accept(v);
        v->VisitConstant(this);
    }

    bool isCharConstant() const { return (mCC) ? true : false; }
    bool isStringConstant() const { return (mSC) ? true: false; }
    bool isNumericConstant() const { return (mNC) ? true: false; }
    const CharConstant* getCharConstant() const { return mCC.get(); }
    const NumericConstant* getNumericConstant() const { return mNC.get(); }
    const StringConstant* getStringConstant() const { return mSC.get(); }

    Type getType() const { return mType; }

    string toString() const { return mStr; }
    void setString(const string& str) { mStr = str; }

    TokenType getTokenType() const {
    	if(isCharConstant())
    		return TOK_CHAR;
    	if(isStringConstant())
    		return TOK_STRING;
    	if(isNumericConstant()) {
    		if(mNC->isInt())
    			return TOK_INT;
    		if(mNC->isFloat())
    			return TOK_FLOAT;
    	}
    	return TOK_ERR;
    }
};

class Identifier : public TestExpr {
private:
    string mIdentifierString;
public:

    Identifier(const string& str) : mIdentifierString(str) {    }
    Identifier(const Identifier& that)
    : TestExpr(that), mIdentifierString(that.mIdentifierString) {}

    void accept(Visitor *v) {
        v->VisitIdentifier(this);
    }

    string toString() const {return mIdentifierString;}
};

class Operand : public TestExpr {
private:
    unique_ptr<Constant> mC;
    unique_ptr<Identifier> mI;
public:
    explicit Operand(Constant* C) : mC(C), mI(nullptr) {}
    explicit Operand(Identifier* I) : mC(nullptr), mI(I) {}
    Operand(const Operand& that) : TestExpr(that), mC(nullptr), mI(nullptr) {
    	if(that.mC) mC = unique_ptr<Constant>(new Constant(*that.mC));
    	if(that.mI) mI = unique_ptr<Identifier>(new Identifier(*that.mI));
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
    const Identifier* getIdentifier() const { return mI.get();}
    const Constant* getConstant() const { return mC.get(); }
    string toString() const {
    	if(isIdentifier())
    		return mI->toString();
    	if(isConstant())
    		return mC->toString();
    	return "";
    }
};

class ExpectedConstant : public TestExpr {
private:
	unique_ptr<Constant> mC;
    unique_ptr<DataPlaceholder> mDP;
public:
    ExpectedConstant() = delete;
    explicit ExpectedConstant(Constant *C) : mC(C), mDP(nullptr) {}
    explicit ExpectedConstant(unique_ptr<DataPlaceholder> dp) : mC(nullptr), mDP(move(dp)) {}
    ExpectedConstant(const ExpectedConstant& that) : TestExpr(that), mC(nullptr), mDP(nullptr) {
    	if(that.mC) mC = unique_ptr<Constant>(new Constant(*that.mC));
    	if(that.mDP.get())
    		mDP =  unique_ptr<DataPlaceholder>(new DataPlaceholder(*that.mDP.get()));
    }

    void accept(Visitor* v) {
    	if (mC)
    		mC->accept(v);
    	if (mDP.get())
    		mDP->accept(v);
        v->VisitExpectedConstant(this);
    }

    bool isDataPlaceholder() const { return mDP.get() != nullptr; }

    const Constant* getConstant() const { return mC.get(); }
};

class StructInitializer;

class InitializerValue : public TestExpr {
private:
    unique_ptr<NumericConstant> mNC;
    unique_ptr<StructInitializer> mStructValue;
public:
    explicit InitializerValue(NumericConstant* val) : mNC(val), mStructValue(nullptr) {}
    explicit InitializerValue(StructInitializer* val) : mNC(nullptr),
        mStructValue(val) {}
    InitializerValue(const InitializerValue& that);

    void accept(Visitor *v);

    bool isArgument() const { return (mNC) ? true : false; }
    bool isStructInitializer() const { return (mStructValue) ? true : false; }

    // @warning Make sure you call these only when the pointer is not nullptr!
    // use the isArgument() method before.
    const NumericConstant& getArgument() const { return *mNC; }
    const StructInitializer& getStructInitializer() const { return *mStructValue;}
};

class InitializerList : public TestExpr {
private:
    vector<InitializerValue*> mArguments;
public:
    InitializerList(vector<InitializerValue*> & arg) : mArguments(arg) {}
    InitializerList(const InitializerList& that) : TestExpr(that) {
    	for(InitializerValue* iv : that.mArguments)
    		mArguments.push_back(new InitializerValue(*iv));
    }
    ~InitializerList() {
        for(InitializerValue*& ptr : mArguments)
            delete ptr;
    }

    void accept(Visitor *v) {
        for(InitializerValue*& ptr : mArguments)
            ptr->accept(v);
        v->VisitInitializerList(this);
    }

    const vector<tp::InitializerValue*>& getArguments() const { return mArguments; }
};


class DesignatedInitializer : public TestExpr {
private:
    vector<tuple<Identifier*,InitializerValue*>> mInit;
public:
    DesignatedInitializer(vector<tuple<Identifier*,InitializerValue*>> & arg)
            : mInit(arg) {}
    DesignatedInitializer(const DesignatedInitializer& that) : TestExpr(that){
    	assert(false && "Not supported at the moment");
    }
    ~DesignatedInitializer() {
        Identifier * id = nullptr;
        InitializerValue * arg = nullptr;
        for(tuple<Identifier*,InitializerValue*>& tup : mInit) {
            id = get<0>(tup);
            arg = get<1>(tup);
            delete id;
            delete arg;
        }
    }

    void accept(Visitor *v) {
        Identifier * id = nullptr;
        InitializerValue * arg = nullptr;
        for(tuple<Identifier*,InitializerValue*>& tup : mInit) {
            id = get<0>(tup);
            arg = get<1>(tup);
            id->accept(v);
            arg->accept(v);
        }
        v->VisitDesignatedInitializer(this);
    }

    const vector<tuple<Identifier*,InitializerValue*>>& getInitializers() const { return mInit; }
};

class StructInitializer : public TestExpr{
private:
    unique_ptr<InitializerList> mInitializerList;
    unique_ptr<DesignatedInitializer> mDesignatedInitializer;

public:
    StructInitializer(InitializerList *init)
        : mInitializerList(init), mDesignatedInitializer(nullptr) { }

    StructInitializer(DesignatedInitializer *init)
        : mInitializerList(nullptr), mDesignatedInitializer(init) {}

    StructInitializer(const StructInitializer& that)
    : TestExpr(that),
      mInitializerList(nullptr), mDesignatedInitializer(nullptr) {
    	if (that.mInitializerList)
    		mInitializerList = unique_ptr<InitializerList>(
    				new InitializerList(*that.mInitializerList));
    	if (that.mDesignatedInitializer)
    		mDesignatedInitializer = unique_ptr<DesignatedInitializer>(
    				new DesignatedInitializer(*that.mDesignatedInitializer));
    }

    void accept(Visitor *v) {
        if (mInitializerList) mInitializerList->accept(v);
        if (mDesignatedInitializer) mDesignatedInitializer->accept(v);
        v->VisitStructInitializer(this);
    }

    const InitializerList* getInitializerList() const { return mInitializerList.get(); }
    const DesignatedInitializer* getDesignatedInitializer() const { return mDesignatedInitializer.get(); }
};

class FunctionArgument;

class FunctionCall : public TestExpr {
private:
    unique_ptr<Identifier> mFunctionName;
    vector<FunctionArgument*> mFunctionArguments;
    // owned by llvm, do not delete
    llvm::Type *mReturnType;
public:

    const Identifier *getIdentifier() const {return mFunctionName.get();}

    FunctionCall(Identifier* name, const vector<FunctionArgument*>& arg);
    FunctionCall(const FunctionCall& that);

    virtual ~FunctionCall();

    void accept(Visitor *v);

    string getFunctionCalledString();
    void setReturnType(llvm::Type *type) { mReturnType = type; }
    llvm::Type* getReturnType() const { return mReturnType; }

    bool hasDataPlaceholders() const;
    vector<unsigned> getDataPlaceholdersPos() const;

    bool replaceDataPlaceholder(unsigned pos, FunctionArgument* new_arg);
};

class ExpectedResult : public TestExpr {
private:
    unique_ptr<ComparisonOperator> mCompOp;
    unique_ptr<ExpectedConstant> mEC;
public:
    ExpectedResult(ComparisonOperator* cmp, ExpectedConstant* Arg) :
    mCompOp(cmp), mEC(Arg) {}
    ExpectedResult(const ExpectedResult& that) :
    	TestExpr(that), mCompOp(nullptr), mEC(nullptr){
    	if(that.mCompOp)
    		mCompOp = unique_ptr<ComparisonOperator>(new ComparisonOperator(*that.mCompOp));
    	if(that.mEC)
    		mEC = unique_ptr<ExpectedConstant>(new ExpectedConstant(*that.mEC));
    }

    void accept(Visitor *v) {
        mCompOp->accept(v);
        mEC->accept(v);
        v->VisitExpectedResult(this);
    }

    const ComparisonOperator* getComparisonOperator() const { return mCompOp.get(); }
    const ExpectedConstant* getExpectedConstant() const { return mEC.get(); }

    bool isDataPlaceholder() const {
    	return mEC->isDataPlaceholder();
    }

    bool replaceDataPlaceholder(ExpectedConstant* ER) {
    	if(mEC->isDataPlaceholder() == false)
    		return false;
    	mEC.reset(ER);
    	return true;
    }
};

class ExpectedExpression : public TestExpr, public LLVMFunctionHolder {
private:
    unique_ptr<Operand> mLHS;
    unique_ptr<ComparisonOperator> mCO;
    unique_ptr<Operand> mRHS;
public:
    ExpectedExpression(Operand* LHS, ComparisonOperator* CO, Operand* RHS,
    		int l=0, int c=0) :
    mLHS(LHS), mCO(CO), mRHS(RHS) {
    	setLine(l);
    	setColumn(c);
    	type = TestExpr::EXPECT_EXPR;
    }
    ExpectedExpression(const ExpectedExpression& that)
    : TestExpr(that), mLHS(nullptr), mCO(nullptr), mRHS(nullptr) {
    	mLHS = unique_ptr<Operand>(new Operand(*that.mLHS));
    	mCO = unique_ptr<ComparisonOperator>(new ComparisonOperator(*that.mCO));
    	mRHS = unique_ptr<Operand>(new Operand(*that.mRHS));
    }

    void accept(Visitor* v) {
        mLHS->accept(v);
        mCO->accept(v);
        mRHS->accept(v);
        v->VisitExpectedExpression(this);
    }

    const Operand* getLHSOperand() const { return mLHS.get(); }
    const Operand* getRHSOperand() const { return mRHS.get(); }
    const ComparisonOperator* getComparisonOperator() const { return mCO.get(); }
    string toString() const {
    	string str;
    	str = mLHS->toString() + " " + mCO->toString() + " " + mRHS->toString();
    	return str;
    }
};

class BufferAlloc : public TestExpr {
private:
    // @todo Create and Integer class.
	unique_ptr<NumericConstant> mIntBuffSize;
	unique_ptr<NumericConstant> mIntDefaultValue;
    unique_ptr<StructInitializer> mStructInit;
public:
    BufferAlloc(NumericConstant* size, NumericConstant* default_value = nullptr) :
            mIntBuffSize(size), mIntDefaultValue(default_value),
            mStructInit(nullptr) {
                if (default_value == nullptr)
                    mIntDefaultValue = unique_ptr<NumericConstant>(
                    		new NumericConstant(0));
    }

    BufferAlloc(NumericConstant* size, StructInitializer* init):
            mIntBuffSize(size), mIntDefaultValue(nullptr),  mStructInit(init)  {

    }

    BufferAlloc(const BufferAlloc& that)
    : TestExpr(that), mIntBuffSize(nullptr), mIntDefaultValue(nullptr),
      mStructInit(nullptr) {
    	if (that.mIntBuffSize)
    		mIntBuffSize = unique_ptr<NumericConstant>(
    				new NumericConstant(*that.mIntBuffSize));
    	if (that.mIntDefaultValue)
    		mIntDefaultValue = unique_ptr<NumericConstant>(
    				new NumericConstant(*that.mIntDefaultValue));
    	if (that.mStructInit)
    		mStructInit = unique_ptr<StructInitializer>(
    				new StructInitializer(*that.mStructInit));
    }

    bool isAllocatingStruct() const { return (mStructInit) ? true : false; }

    const StructInitializer* getStructInitializer() const { return mStructInit.get(); }

    void accept(Visitor *v) {
        mIntBuffSize->accept(v);
        if (mIntDefaultValue) mIntDefaultValue->accept(v);
        if (mStructInit) mStructInit->accept(v);
        v->VisitBufferAlloc(this); // Doesn't have any children
    }

    unsigned getBufferSize() const {
        return mIntBuffSize->getInt();
    }
    string getBufferSizeAsString() const {
        return mIntBuffSize->toString();
    }

    string getDefaultValueAsString() const {
        return mIntDefaultValue->toString();
    }
};

class VariableAssignment : public TestExpr {
private:
    Identifier *mIdentifier;
    Constant *mArgument;
    StructInitializer *mStructInitializer;
    BufferAlloc *mBufferAlloc;
public:

    explicit VariableAssignment(Identifier *id, Constant *arg) :
    mIdentifier(id), mArgument(arg), mStructInitializer(nullptr), mBufferAlloc(nullptr) {
    	type = TestExpr::VAR_ASSIGN;
    }

    explicit  VariableAssignment(Identifier *id, StructInitializer *arg) :
    mIdentifier(id), mArgument(nullptr), mStructInitializer(arg), mBufferAlloc(nullptr) {
    	type = TestExpr::VAR_ASSIGN;
    }

    explicit VariableAssignment(Identifier *id, BufferAlloc *ba) :
    mIdentifier(id), mArgument(nullptr), mStructInitializer(nullptr), mBufferAlloc(ba) {
    	type = TestExpr::VAR_ASSIGN;
    }

    VariableAssignment(const VariableAssignment& that) :
    TestExpr(that), mIdentifier(nullptr), mArgument(nullptr),
    mStructInitializer(nullptr), mBufferAlloc(nullptr) {
    	if(that.mIdentifier) mIdentifier = new Identifier(*that.mIdentifier);
    	if(that.mArgument) mArgument = new Constant(*that.mArgument);
    	if(that.mStructInitializer)
    		mStructInitializer = new StructInitializer(*that.mStructInitializer);
    	if(that.mBufferAlloc)
    		mBufferAlloc = new BufferAlloc(*that.mBufferAlloc);
    }

    ~VariableAssignment() {
        delete mIdentifier;
        if (mArgument) delete mArgument;
        if (mStructInitializer) delete mStructInitializer;
        if (mBufferAlloc) delete mBufferAlloc;
    }

    Identifier* getIdentifier() const { return mIdentifier; }
    Constant* getConstant() const { return mArgument; }
    StructInitializer* getStructInitializer() const { return mStructInitializer; }
    BufferAlloc* getBufferAlloc() const { return mBufferAlloc; }

    bool isArgument() const { return (mArgument) ? true : false; }
    bool isStructInitializer() const { return (mStructInitializer) ? true : false; }
    bool isBufferAlloc() const { return (mBufferAlloc) ? true : false; }
    TokenType getTokenType() const {
        if (mArgument)
            return mArgument->getTokenType();
        if (mBufferAlloc)
            return TOK_BUFF_ALLOC;
        if (mStructInitializer)
            return TOK_STRUCT_INIT;
        return TOK_ERR;
    }

    void accept(Visitor *v){
        mIdentifier->accept(v);
        if (mArgument) mArgument->accept(v);
        if (mStructInitializer) mStructInitializer->accept(v);
        if (mBufferAlloc) mBufferAlloc->accept(v);
        v->VisitVariableAssignment(this);
    }
};

class MockupVariable : public TestExpr {
private:
    unique_ptr<VariableAssignment> mVariableAssignment;
public:

    MockupVariable(VariableAssignment *var) : mVariableAssignment(var) {
        if (mVariableAssignment == nullptr)
            throw Exception("Invalid Variable Assignment Expression type");
    }

    MockupVariable(const MockupVariable& that)
    : TestExpr(that), mVariableAssignment(nullptr) {
    	mVariableAssignment = unique_ptr<VariableAssignment>(
    			new VariableAssignment(*that.mVariableAssignment));
    }

    void accept(Visitor *v) {
        mVariableAssignment->accept(v);
        v->VisitMockupVariable(this);
    }
};

class MockupFunction : public TestExpr {
private:
    unique_ptr<FunctionCall> mFunctionCall;
    unique_ptr<Constant> mConstant;
    // owned by llvm, do not delete!
    llvm::Function *mOriginalFunction;
    llvm::Function *mMockupFunction;
public:
    MockupFunction(FunctionCall *call, Constant *arg) :
    mFunctionCall(call), mConstant(arg),
    mOriginalFunction(nullptr), mMockupFunction(nullptr) { }

    MockupFunction(const MockupFunction& that)
    : TestExpr(that),  mFunctionCall(nullptr), mConstant(nullptr),
     mOriginalFunction(nullptr), mMockupFunction(nullptr) {
    	mFunctionCall = unique_ptr<FunctionCall>(
    			new FunctionCall(*that.mFunctionCall));
    	mConstant = unique_ptr<Constant>(
    	    			new Constant(*that.mConstant));
    }

    const FunctionCall* getFunctionCall() const { return mFunctionCall.get(); }
    const Constant* getArgument() const { return mConstant.get(); }

    void accept(Visitor *v) {
        v->VisitMockupFunction(this);
    }

    llvm::Function* getMockupFunction() const {
        return mMockupFunction;
    }

    void setMockupFunction(llvm::Function* mMockupFunction) {
        this->mMockupFunction = mMockupFunction;
    }

    llvm::Function* getOriginalFunction() const {
        return mOriginalFunction;
    }

    void setOriginalFunction(llvm::Function* mOriginalFunction) {
        this->mOriginalFunction = mOriginalFunction;
    }
};

class MockupFixture : public TestExpr {
private:
    vector<MockupFunction*> mMockupFunctions;
    vector<MockupVariable*> mMockupVariables;
public:
    // @todo @warning @bug Copy this object when using a DataPlaceHolder!

    MockupFixture(const vector<MockupFunction*>& func,
            const vector<MockupVariable*>& var) :
    mMockupFunctions(func), mMockupVariables(var) {
    }
    MockupFixture(const MockupFixture& that) :
    	TestExpr(that),	mMockupFunctions(), mMockupVariables()
    {
    	for (MockupFunction* ptr : that.mMockupFunctions)
			mMockupFunctions.push_back(new MockupFunction(*ptr));

		for (MockupVariable* ptr : that.mMockupVariables)
			mMockupVariables.push_back(new MockupVariable(*ptr));
    }

    ~MockupFixture() {
        for (auto*& ptr : mMockupFunctions)
            delete ptr;

        for (auto*& ptr : mMockupVariables)
            delete ptr;
    }

    void accept(Visitor *v) {
        for (auto*& ptr : mMockupVariables)
            ptr->accept(v);

        for (auto*& ptr : mMockupFunctions)
            ptr->accept(v);

        v->VisitMockupFixture(this);
    }

    vector<MockupFunction*> getMockupFunctions() const { return mMockupFunctions; }
};

class TestMockup : public TestExpr {
private:
    unique_ptr<MockupFixture> mMockupFixture;
public:
    // @todo @warning @bug Copy this object when using a DataPlaceHolder!
    TestMockup(MockupFixture *fixt) : mMockupFixture(fixt) { }
    TestMockup(const TestMockup& that) : TestExpr(that), mMockupFixture(nullptr) {
    	mMockupFixture = unique_ptr<MockupFixture>(
    			new MockupFixture(*that.mMockupFixture));
    }

    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitTestMockup(this);
    }

    const MockupFixture* getMockupFixture() const { return mMockupFixture.get(); }
};

class TestFixture : public TestExpr {
private:
    vector<TestExpr*> mStmt;
public:

    TestFixture(const vector<TestExpr*>& stmnt) : mStmt(stmnt) { }
    /// @warning @bug Watch for possible bugs here. We may need tod convert
    /// to that derived type first in order to copy!
    TestFixture(const TestFixture& that) : TestExpr(that) {
    	for(TestExpr* t : that.mStmt) {
    		if(t->getType() == TestExpr::FUNC_CALL) {
    			FunctionCall* fc = static_cast<FunctionCall*>(t);
    			mStmt.push_back(new FunctionCall(*fc));
    			continue;
    		}
			if(t->getType() == TestExpr::VAR_ASSIGN) {
				VariableAssignment* va = static_cast<VariableAssignment*>(t);
				    			mStmt.push_back(new VariableAssignment(*va));
				continue;
			}
			if(t->getType() == TestExpr::EXPECT_EXPR) {
				ExpectedExpression* ee = static_cast<ExpectedExpression*>(t);
				    			mStmt.push_back(new ExpectedExpression(*ee));
				continue;
			}
			if(t->getType() == TestExpr::OTHER)
				assert(false && "Fatal error!");
    	}
    }

    ~TestFixture() {
        for (auto*& ptr : mStmt)
            delete ptr;
    }

    void accept(Visitor *v) {
        for (auto*& ptr : mStmt)
            ptr->accept(v);
        v->VisitTestFixture(this);
    }
};

class TestSetup : public TestExpr {
private:
    unique_ptr<TestFixture> mTestFixtureExpr;
public:
    TestSetup(TestFixture* fixture) : mTestFixtureExpr(fixture) {}
    TestSetup(const TestSetup& that) :
    	TestExpr(that), mTestFixtureExpr(nullptr) {
    	if (that.mTestFixtureExpr)
    		mTestFixtureExpr = unique_ptr<TestFixture>(
    				new TestFixture(*that.mTestFixtureExpr));
    }

    void accept(Visitor *v) {
        mTestFixtureExpr->accept(v);
        v->VisitTestSetup(this);
    }
};

class TestFunction : public TestExpr {
private:
    unique_ptr<FunctionCall> mFunctionCall;
    unique_ptr<ExpectedResult> mExpectedResult;
public:
    TestFunction(FunctionCall *F, ExpectedResult* E=nullptr) : mFunctionCall(F),
            mExpectedResult(E) {}
    TestFunction(const TestFunction& that)
    : TestExpr(that), mFunctionCall(nullptr), mExpectedResult(nullptr) {
    	if(that.mFunctionCall)
    		mFunctionCall = unique_ptr<FunctionCall>(
    				new FunctionCall(*that.mFunctionCall));
    	if(that.mExpectedResult)
    		mExpectedResult = unique_ptr<ExpectedResult>(
    				new ExpectedResult(*that.mExpectedResult));
    }

    FunctionCall* getFunctionCall() const {return mFunctionCall.get(); }
    ExpectedResult* getExpectedResult() const {return mExpectedResult.get(); }

    void accept(Visitor *v) {
        mFunctionCall->accept(v);
        if (mExpectedResult)
            mExpectedResult->accept(v);
        v->VisitTestFunction(this);
    }

    bool hasDataPlaceholders() const {
    	bool p1 = mFunctionCall->hasDataPlaceholders();
    	bool p2 = (mExpectedResult) ? mExpectedResult->isDataPlaceholder() : false;
    	return  p1 || p2;
    }
};

class TestTeardown : public TestExpr {
private:
    unique_ptr<TestFixture> mTestFixture;
public:
    TestTeardown(TestFixture* fixture) : mTestFixture(fixture) {}
    TestTeardown(const TestTeardown& that)
    : TestExpr(that), mTestFixture(nullptr){
    	if(that.mTestFixture)
    		mTestFixture = unique_ptr<TestFixture>(
    				new TestFixture(*that.mTestFixture));
    }

    void accept(Visitor *v) {
        mTestFixture->accept(v);
        v->VisitTestTeardown(this);
    }
};

class TestData : public TestExpr {
private:
    unique_ptr<StringConstant> mDataPath;
public:
    TestData(unique_ptr<StringConstant> path) : mDataPath(move(path)) {}
    TestData(const TestData& that) : TestExpr(that), mDataPath(nullptr) {
    	if(that.mDataPath.get())
    		mDataPath = unique_ptr<StringConstant>
    					(new StringConstant(*that.mDataPath.get()));
    }

    void accept(Visitor* v) {
        v->VisitTestInfo(this);
    }

    string getDataPath() const {
    	const string& path = mDataPath->getString();
    	return path.substr(1,path.size()-2); // Remove the double quotes
    }
};

class TestDefinition : public TestExpr, public LLVMFunctionHolder {
private:
    unique_ptr<TestData> mTestData;
    unique_ptr<TestFunction> mTestFunction;
    unique_ptr<TestSetup> mTestSetup;
    unique_ptr<TestTeardown> mTestTeardown;
    unique_ptr<TestMockup> mTestMockup;
    // Do not delete these pointers! They belong to someone else!
	// We only store ExpectedExpressions that are known to have failed.
	std::vector<ExpectedExpression*> mFailedEE;
public:

    TestDefinition(
            TestData *info,
            TestFunction *function,
            TestSetup *setup = nullptr,
            TestTeardown *teardown = nullptr,
            TestMockup *mockup = nullptr) :
    mTestData(info), mTestFunction(function), mTestSetup(setup),
    mTestTeardown(teardown), mTestMockup(mockup) { }

    TestDefinition(const TestDefinition& that)
    : TestExpr(that), mTestData(nullptr), mTestFunction(nullptr), mTestSetup(nullptr),
      mTestTeardown(nullptr), mTestMockup(nullptr), mFailedEE(that.mFailedEE) {
    	if(that.mTestData)
    		mTestData = unique_ptr<TestData>(new TestData(*that.mTestData));
    	if(that.mTestFunction)
    		mTestFunction = unique_ptr<TestFunction>(
    						new TestFunction(*that.mTestFunction));
    	if(that.mTestSetup)
    		mTestSetup = unique_ptr<TestSetup>(
    					 new TestSetup(*that.mTestSetup));
    	if(that.mTestTeardown)
    		mTestTeardown = unique_ptr<TestTeardown>(
    						new TestTeardown(*that.mTestTeardown));
    	if(that.mTestMockup)
    		mTestMockup = unique_ptr<TestMockup>(
    					  new TestMockup(*that.mTestMockup));
    }

    TestData* getTestData() const { return mTestData.get(); }
    TestFunction * getTestFunction() const {return mTestFunction.get();}
    TestMockup* getTestMockup() const { return mTestMockup.get(); }

    bool hasTestMockup() const { return mTestMockup != nullptr; }

    void accept(Visitor *v) {
        v->VisitTestDefinitionFirst(this);
        if(mTestData) mTestData->accept(v);
        if(mTestMockup) mTestMockup->accept(v);
        if(mTestSetup) mTestSetup->accept(v);
        mTestFunction->accept(v);
        if(mTestTeardown) mTestTeardown->accept(v);
        v->VisitTestDefinition(this);
    }

    void setFailedExpectedExpressions(std::vector<ExpectedExpression*> failed) {
    	mFailedEE = failed;
    }

       const std::vector<ExpectedExpression*>&
       getFailedExpectedExpressions() const { return mFailedEE; }

    bool testPassed() const {
    	bool passed = getPassingValue();
    	if(mFailedEE.size())
    		passed = false;
    	return passed;
    }
};

class GlobalMockup : public TestExpr {
private:
    unique_ptr<MockupFixture> mMockupFixture;
public:
    GlobalMockup(const MockupFixture&) = delete;
    GlobalMockup(MockupFixture *fixt) : mMockupFixture(fixt) {    }

    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitGroupMockup(this);
    }

    MockupFixture* getMockupFixture() const { return mMockupFixture.get(); }
};

class GlobalSetup : public TestExpr, public LLVMFunctionHolder {
private:
    unique_ptr<TestFixture> mTestFixture;
public:
    GlobalSetup(const GlobalSetup&) = delete;
    GlobalSetup(TestFixture *fixt) : mTestFixture(fixt) {    }

    void accept(Visitor *v) {
        v->VisitGroupSetupFirst(this);
        mTestFixture->accept(v);
        v->VisitGroupSetup(this);
    }
};

class GlobalTeardown : public TestExpr, public LLVMFunctionHolder {
private:
	unique_ptr<TestFixture> mTestFixture;
public:
	GlobalTeardown(const TestFixture&) = delete;
    GlobalTeardown(TestFixture *fixt) : mTestFixture(fixt) { }

    void accept(Visitor *v) {
        v->VisitGroupTeardownFirst(this);
        if (mTestFixture) mTestFixture->accept(v);
        v->VisitGroupTeardown(this);
    }
};

class TestGroup : public TestExpr, public LLVMFunctionHolder {
private:
    unique_ptr<Identifier> mName;
    // Let's work with tests as we find them, do not alter their order.
    // @todo Change this to vector<unique_ptr<TestExpr>>
    vector<TestExpr*> mTests;
    unique_ptr<GlobalMockup> mGlobalMockup;
    unique_ptr<GlobalSetup> mGlobalSetup;
    unique_ptr<GlobalTeardown> mGlobalTeardown;
    static int group_count;
public:
    TestGroup(Identifier* name,
            const vector<TestExpr*>& tests,
            GlobalMockup *gm = nullptr,
            GlobalSetup *gs = nullptr, GlobalTeardown *gt = nullptr) :
    mName(name), mTests(tests),
    mGlobalMockup(gm), mGlobalSetup(gs), mGlobalTeardown(gt) {
        /// @todo Enable GlobalMockup
        // if (GlobalMockup) GlobalMockup->setGroupName(mName->getIdentifierStr());
        if (mGlobalSetup) mGlobalSetup->setGroupName(mName->toString());
        if (mGlobalTeardown) mGlobalTeardown->setGroupName(mName->toString());
    }
    ~TestGroup() {
        for (auto*& ptr : mTests)
            delete ptr;
    }

    void accept(Visitor *v) {
        v->VisitTestGroupFirst(this);
        if (mName) mName->accept(v);
        if (mGlobalMockup) mGlobalMockup->accept(v);
        if (mGlobalSetup) mGlobalSetup->accept(v);
        for (auto*& ptr : mTests) {
            ptr->accept(v);
        }
        if (mGlobalTeardown) mGlobalTeardown->accept(v);
        v->VisitTestGroup(this);
    }

    string getGroupName() const { return mName->toString(); }

    const GlobalSetup* getGlobalSetup() const { return mGlobalSetup.get(); }
    const GlobalTeardown* getGlobalTeardown() const { return mGlobalTeardown.get(); }
    const GlobalMockup* getGlobalMockup() const { return mGlobalMockup.get(); }
    void setGlobalTeardown(GlobalTeardown* gt) { mGlobalTeardown.reset(gt); }
    // Use it with wisdom, you can modify the internal object structure.
    vector<TestExpr*>& getTests() { return mTests; }
};

class TestFile : public TestExpr {
private:
    unique_ptr<TestGroup> mTestGroups;
public:
    TestFile(const TestFile&) = delete;
    TestFile(TestGroup *tg) : mTestGroups(tg) { }

    void accept(Visitor *v) {
        v->VisitTestFileFirst(this);
        mTestGroups->accept(v);
        v->VisitTestFile(this);
    }
};

class FunctionArgument : public TestExpr{
protected:
	unique_ptr<Constant> mConstant;
	unique_ptr<BufferAlloc> argBuffAlloc;
    unique_ptr<DataPlaceholder> mDP;
    unsigned    ArgIndx;
public:
    explicit FunctionArgument(Constant *arg) :
        mConstant(arg), argBuffAlloc(nullptr), mDP(nullptr), ArgIndx(0) { }
    explicit FunctionArgument(BufferAlloc *arg) :
        mConstant(nullptr), argBuffAlloc(arg), mDP(nullptr),ArgIndx(0) { }
    explicit FunctionArgument(unique_ptr<DataPlaceholder> dp) :
            mConstant(nullptr), argBuffAlloc(nullptr), mDP(move(dp)), ArgIndx(0) { }
    FunctionArgument(const FunctionArgument& that): TestExpr(that),
    	mConstant(nullptr), argBuffAlloc(nullptr), mDP(nullptr), ArgIndx(that.ArgIndx) {
    	if(that.mConstant)
    		mConstant = unique_ptr<Constant>(new Constant(*that.mConstant));
    	if(that.argBuffAlloc)
    		argBuffAlloc = unique_ptr<BufferAlloc>(new BufferAlloc(*that.argBuffAlloc));
    	if(that.mDP.get())
    		mDP = unique_ptr<DataPlaceholder>(new DataPlaceholder(*that.mDP.get()));
    	ArgIndx = that.ArgIndx;
    }

    bool isBufferAlloc() const { return argBuffAlloc != nullptr; }
    bool isDataPlaceholder() const { return mDP.get() != nullptr; }
    TokenType getTokenType() const {
        if(mConstant) return mConstant->getTokenType();
        return TOK_BUFF_ALLOC;
    }

    string toString() {
        if(mConstant)
            return mConstant->toString();
        if(mDP.get())
        	return "@";
        return "BufferAlloc";
    }
    unsigned getIndex() const { return ArgIndx; }
    void setIndex(unsigned ndx) {ArgIndx = ndx; }

    const BufferAlloc* getBufferAlloc() const { return argBuffAlloc.get(); }

    void accept(Visitor *v) {
        if(mConstant)
            mConstant->accept(v);
        if(argBuffAlloc)
            argBuffAlloc->accept(v);

        v->VisitFunctionArgument(this);// TODO IMplement
    }

	const Constant* getArgument() const {
		return mConstant.get();
	}

};

class TestDriver {
public:
    TestDriver() {}
    TestDriver(const TestDriver&) = delete;

    TestDriver(const string& file) : mTokenizer(),
    mCurrentToken() {
    	mTokenizer.tokenize(file);
    }

    void tokenize(const string& file) {
    	mTokenizer.tokenize(file);
	}

	void tokenize(const char* buffer) {
		mTokenizer.tokenize(buffer);
	}

    ~TestDriver() { }

    TestExpr* ParseTestExpr();

protected:
    Tokenizer mTokenizer;
	Token mCurrentToken;

    /// Generates default names for groups
    Identifier* groupNameFactory();
    unique_ptr<DataPlaceholder> ParseDataPlaceholder();
    Identifier* ParseIdentifier();
    InitializerValue* ParseInitializerValue();
    DesignatedInitializer* ParseDesignatedInitializer();
    InitializerList* ParseInitializerList();
    StructInitializer* ParseStructInitializer();
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
    StringConstant* ParseStringConstant();
    Constant* ParseConstant();
    NumericConstant* ParseNumericConstant();
    CharConstant* ParseCharConstant();
    TestTeardown* ParseTestTearDown();
    TestFunction* ParseTestFunction();
    TestSetup* ParseTestSetup();
    TestFixture* ParseTestFixture();
    MockupVariable* ParseMockupVariable();
    MockupFunction* ParseMockupFunction();
    MockupFixture* ParseMockupFixture();
    TestMockup* ParseTestMockup();
    TestData* ParseTestData();
    TestDefinition* ParseTestDefinition();
    // @arg name The name of the group to be parsed
    TestGroup* ParseTestGroup(Identifier* name);
    GlobalMockup* ParseGroupMockup();
    GlobalSetup* ParseGroupSetup();
    GlobalTeardown* ParseGroupTeardown();
    TestFile* ParseTestFile();
};

class CSVDriver : public TestDriver{
public:
	CSVDriver(const string& filename);

	unsigned columnCount();
	unsigned rowCount();
	FunctionArgument* getFunctionArgumentAt(unsigned i, unsigned j);
	ExpectedConstant* getExpectedConstantAt(unsigned i, unsigned j);
private:
	unsigned rows;
	unsigned columns;
	vector<string> split(string line, char split_char);
	vector<vector<string>> mMatrix;
};

} // namespace tp
#endif	/* TESTPARSER_H */
