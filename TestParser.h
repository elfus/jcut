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
    : mStringRepresentation(that.mStringRepresentation), mType(that.mType) {}

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
    : mIC(that.mIC), mFC(that.mFC), mIsInt(that.mIsInt) {}
    ~NumericConstant() {}

    void accept(Visitor* v) {
        v->VisitNumericConstant(this);
    }

    int getInt() const { return mIC; }
    float getFloat() const { return mFC; }
    bool isInt() const { return mIsInt; }
    bool isFloat() const { return !mIsInt; }
};

class StringConstant : public TestExpr{
private:
    string mStr;
public:
    StringConstant(const string& str) : mStr(str) {}
    StringConstant(const StringConstant& that) : mStr(that.mStr) {}

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
    CharConstant(const CharConstant& that) : mC(that.mC) {}
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
    NumericConstant* mNC;
    StringConstant* mSC;
    CharConstant* mCC;
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
    : mNC(nullptr), mSC(nullptr), mCC(nullptr), mType(INVALID) {
    	if(that.mNC) {
    		mNC = new NumericConstant(*that.mNC);
    		stringstream ss;
    		if(mNC->isInt())
				ss << mNC->getInt();
			if(mNC->isFloat())
				ss << mNC->getFloat();
			mStr = ss.str();
    	}
    	if(that.mSC) mSC = new StringConstant(*that.mSC);
    	if(that.mCC) mCC = new CharConstant(*that.mCC);
    	mType = that.mType;
    }
    ~Constant() {
        if(mNC) delete mNC;
        if(mSC) delete mSC;
        if(mCC) delete mCC;
    }

    void accept(Visitor* v) {
        if(mNC) mNC->accept(v);
        if(mSC) mSC->accept(v);
        if(mCC) mCC->accept(v);
        v->VisitConstant(this);
    }

    // @deprecated do not use
    int getValue() const {
        // @bug remove mNC->getInt()
        if(mNC) return mNC->getInt();
        // @bug String not supported yet
        if(mCC) return (int) mCC->getChar();
        return 0;
    }

    bool isCharConstant() const { return (mCC) ? true : false; }
    bool isStringConstant() const { return (mSC) ? true: false; }
    bool isNumericConstant() const { return (mNC) ? true: false; }
    CharConstant* getCharConstant() const { return mCC; }
    NumericConstant* getNumericConstant() const { return mNC; }
    StringConstant* getStringConstant() const { return mSC; }

    Type getType() const { return mType; }

    // @todo Rename getAsStr method to something else
    string getAsStr() const { return mStr; }
    void setAsStr(const string& str) { mStr = str; }
};

class Identifier : public TestExpr {
private:
    string mIdentifierString;
public:

    Identifier(const string& str) : mIdentifierString(str) {    }
    Identifier(const Identifier& that)
    : mIdentifierString(that.mIdentifierString) {}

    ~Identifier() {    }

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
    Operand(const Operand& that) : mC(nullptr), mI(nullptr) {
    	if(that.mC) mC = new Constant(*that.mC);
    	if(that.mI) mI = new Identifier(*that.mI);
    }
    ~Operand() {
        if (mC)
            delete mC;
        if(mI)
            delete mI;
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
    string toString() const {
    	if(isIdentifier())
    		return mI->getIdentifierStr();

    	if(isConstant())
    		return mC->getAsStr();

    	return "";
    }
};

class ExpectedConstant : public TestExpr {
private:
    Constant* mC;
    unique_ptr<DataPlaceholder> mDP;
public:
    ExpectedConstant() = delete;
    explicit ExpectedConstant(Constant *C) : mC(C), mDP(nullptr) {}
    explicit ExpectedConstant(unique_ptr<DataPlaceholder> dp) : mC(nullptr), mDP(move(dp)) {}
    ExpectedConstant(const ExpectedConstant& that) : mC(nullptr), mDP(nullptr) {
    	if(that.mC) mC = new Constant(*that.mC);
    	if(that.mDP.get())
    		mDP =  unique_ptr<DataPlaceholder>(new DataPlaceholder(*that.mDP.get()));
    }
    ~ExpectedConstant() { delete mC;}

    void accept(Visitor* v) {
    	if (mC)
    		mC->accept(v);
    	if (mDP.get())
    		mDP->accept(v);
        v->VisitExpectedConstant(this);
    }

    int getValue() const {
        return mC->getValue();
    }

    bool isDataPlaceholder() const { return mDP.get() != nullptr; }

    Constant* getConstant() const { return mC; }
};

class Argument : public TestExpr {
protected:
    string StringRepresentation;
    TokenType mTokenType;
public:

    Argument(const string& str, TokenType token) :
    StringRepresentation(str), mTokenType(token) {
        if (mTokenType == TOK_CHAR) {
            // Convert the char to its integer value
            // then store that integer value as string
            // the flex parser takes the single quotes too
            // @bug what happens when we pass \n or any escaped character?
            unsigned char c = StringRepresentation[1];
            unsigned int tmp = static_cast<unsigned int>(c);
            stringstream ss;
            ss << tmp;
            StringRepresentation = ss.str();
        }
    }
    Argument(const Argument& that)
    : StringRepresentation(that.StringRepresentation),
      mTokenType(that.mTokenType) {}

    ~Argument() {}

    const string& getStringRepresentation() const { return StringRepresentation; }
    TokenType getTokenType() const { return mTokenType; }

    void accept(Visitor *v) {
       v->VisitArgument(this); // Argument doesn't have any children
    }
};

class StructInitializer;

class InitializerValue : public TestExpr {
private:
    Argument* mArgValue;
    StructInitializer* mStructValue;
public:
    explicit InitializerValue(Argument* val) : mArgValue(val), mStructValue(nullptr) {}
    explicit InitializerValue(StructInitializer* val) : mArgValue(nullptr),
        mStructValue(val) {}
    InitializerValue(const InitializerValue& that);
    ~InitializerValue();

    void accept(Visitor *v);

    bool isArgument() const { return (mArgValue) ? true : false; }
    bool isStructInitializer() const { return (mStructValue) ? true : false; }

    // @warning Make sure you call these only when the pointer is not nullptr!
    // use the isArgument() method before.
    const Argument& getArgument() const { return *mArgValue; }
    const StructInitializer& getStructInitializer() const { return *mStructValue;}
};

class InitializerList : public TestExpr {
private:
    vector<InitializerValue*> mArguments;
public:
    InitializerList(vector<InitializerValue*> & arg) : mArguments(arg) {}
    InitializerList(const InitializerList& that) {
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
    DesignatedInitializer(const DesignatedInitializer& that) {
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
    InitializerList *mInitializerList;
    DesignatedInitializer *mDesignatedInitializer;

public:
    StructInitializer(InitializerList *init)
        : mInitializerList(init), mDesignatedInitializer(nullptr) { }

    StructInitializer(DesignatedInitializer *init)
        : mInitializerList(nullptr), mDesignatedInitializer(init) {}

    StructInitializer(const StructInitializer& that)
    : mInitializerList(nullptr), mDesignatedInitializer(nullptr) {
    	if (that.mInitializerList)
    		mInitializerList = new InitializerList(*that.mInitializerList);
    	if (that.mDesignatedInitializer)
    		mDesignatedInitializer = new DesignatedInitializer
    								 (*that.mDesignatedInitializer);
    }

    ~StructInitializer() {
        if (mInitializerList) delete mInitializerList;
        if (mDesignatedInitializer) delete mDesignatedInitializer;
    }

    void accept(Visitor *v) {
        if (mInitializerList) mInitializerList->accept(v);
        if (mDesignatedInitializer) mDesignatedInitializer->accept(v);
        v->VisitStructInitializer(this);
    }

    InitializerList* getInitializerList() const { return mInitializerList; }
    DesignatedInitializer* getDesignatedInitializer() const { return mDesignatedInitializer; }
};

class FunctionArgument;

class FunctionCall : public TestExpr {
private:
    Identifier *mFunctionName;
    vector<FunctionArgument*> mFunctionArguments;
    // owned by llvm, do not delete
    llvm::Type *mReturnType;
public:

    Identifier *getIdentifier() const {return mFunctionName;}

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
    ComparisonOperator* mCompOp;
    ExpectedConstant* mEC;
public:
    ExpectedResult(ComparisonOperator* cmp, ExpectedConstant* Arg) :
    mCompOp(cmp), mEC(Arg) {}
    ExpectedResult(const ExpectedResult& that) : mCompOp(nullptr), mEC(nullptr){
    	if(that.mCompOp)
    		mCompOp = new ComparisonOperator(*that.mCompOp);
    	if(that.mEC)
    		mEC = new ExpectedConstant(*that.mEC);
    }
    ~ExpectedResult() {
        delete mEC;
        delete mCompOp;
    }

    void accept(Visitor *v) {
        mCompOp->accept(v);
        mEC->accept(v);
        v->VisitExpectedResult(this);
    }

    ComparisonOperator* getComparisonOperator() const { return mCompOp; }
    ExpectedConstant* getExpectedConstant() const { return mEC; }

    bool isDataPlaceholder() const {
    	return mEC->isDataPlaceholder();
    }

    bool replaceDataPlaceholder(ExpectedConstant* ER) {
    	if(mEC->isDataPlaceholder() == false)
    		return false;
    	delete mEC;
    	mEC = ER;
    	return true;
    }
};

class ExpectedExpression : public TestExpr, public LLVMFunctionHolder {
private:
    Operand* mLHS;
    ComparisonOperator* mCO;
    Operand* mRHS;
public:
    ExpectedExpression(Operand* LHS, ComparisonOperator* CO, Operand* RHS,
    		int l=0, int c=0) :
    mLHS(LHS), mCO(CO), mRHS(RHS) {
    	setLine(l);
    	setColumn(c);
    	type = TestExpr::EXPECT_EXPR;
    }
    ExpectedExpression(const ExpectedExpression& that)
    : mLHS(nullptr), mCO(nullptr), mRHS(nullptr) {
    	mLHS = new Operand(*that.mLHS);
    	mCO = new ComparisonOperator(*that.mCO);
    	mRHS = new Operand(*that.mRHS);
    }
    ~ExpectedExpression() {
        delete mLHS;
        delete mCO;
        delete mRHS;
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
    string toString() const {
    	string str;
    	str = mLHS->toString() + " " + mCO->toString() + " " + mRHS->toString();
    	return str;
    }
};

class BufferAlloc : public TestExpr {
private:
    // @todo Create and Integer class.
    Argument* mIntBuffSize;
    Argument* mIntDefaultValue;
    StructInitializer* mStructInit;
public:
    BufferAlloc(Argument* size, Argument* default_value = nullptr) :
            mIntBuffSize(size), mIntDefaultValue(default_value),
            mStructInit(nullptr) {
                if (default_value == nullptr)
                    mIntDefaultValue = new Argument("0", TOK_INT);
    }

    BufferAlloc(Argument* size, StructInitializer* init):
            mIntBuffSize(size), mIntDefaultValue(nullptr),  mStructInit(init)  {

    }

    BufferAlloc(const BufferAlloc& that)
    : mIntBuffSize(nullptr), mIntDefaultValue(nullptr), mStructInit(nullptr) {
    	if (that.mIntBuffSize)
    		mIntBuffSize = new Argument(*that.mIntBuffSize);
    	if (that.mIntDefaultValue)
    		mIntDefaultValue = new Argument(*that.mIntDefaultValue);
    	if (that.mStructInit)
    		mStructInit = new StructInitializer(*that.mStructInit);
    }

    ~BufferAlloc() {
        if (mIntBuffSize) delete mIntBuffSize;
        if (mIntDefaultValue) delete mIntDefaultValue;
        if (mStructInit) delete mStructInit;
    }

    bool isAllocatingStruct() const { return (mStructInit) ? true : false; }

    const StructInitializer* getStructInitializer() const { return mStructInit; }

    void accept(Visitor *v) {
        mIntBuffSize->accept(v);
        if (mIntDefaultValue) mIntDefaultValue->accept(v);
        if (mStructInit) mStructInit->accept(v);
        v->VisitBufferAlloc(this); // Doesn't have any children
    }

    unsigned getBufferSize() const {
        stringstream ss;
        ss << mIntBuffSize->getStringRepresentation();
        unsigned tmp;
        ss >> tmp;
        return tmp;
    }
    string getBufferSizeAsString() const {
        return mIntBuffSize->getStringRepresentation();
    }

    string getDefaultValueAsString() const {
        return mIntDefaultValue->getStringRepresentation();
    }
};

class VariableAssignment : public TestExpr {
private:
    Identifier *mIdentifier;
    Argument *mArgument;
    StructInitializer *mStructInitializer;
    BufferAlloc *mBufferAlloc;
public:

    explicit VariableAssignment(Identifier *id, Argument *arg) :
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
    mIdentifier(nullptr), mArgument(nullptr),
    mStructInitializer(nullptr), mBufferAlloc(nullptr) {
    	if(that.mIdentifier) mIdentifier = new Identifier(*that.mIdentifier);
    	if(that.mArgument) mArgument = new Argument(*that.mArgument);
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
    Argument* getArgument() const { return mArgument; }
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
    VariableAssignment *mVariableAssignment;
public:

    MockupVariable(VariableAssignment *var) : mVariableAssignment(nullptr) {
        /// @todo remove dynamic_cast
        mVariableAssignment = dynamic_cast<VariableAssignment*> (var);
        if (mVariableAssignment == nullptr)
            throw Exception("Invalid Variable Assignment Expression type");
    }

    ~MockupVariable() {
        if (mVariableAssignment)
            delete mVariableAssignment;
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
    llvm::Function *mOriginalFunction;
    llvm::Function *mMockupFunction;
public:

    MockupFunction(FunctionCall *call, Argument *arg) :
    mFunctionCall(nullptr), mArgument(nullptr),
    mOriginalFunction(nullptr), mMockupFunction(nullptr) {
        /// @todo remove dynamic_cast
        mFunctionCall = dynamic_cast<FunctionCall*> (call);
        if (mFunctionCall == nullptr)
            throw Exception("Invalid FunctionCallExpr type");
        /// @todo remove dynamic_cast
        mArgument = dynamic_cast<Argument*> (arg);
        if (mArgument == nullptr)
            throw Exception("Invalid Argument type");
    }

    FunctionCall* getFunctionCall() const { return mFunctionCall; }
    Argument* getArgument() const { return mArgument; }

    ~MockupFunction() {
        if (mFunctionCall)
            delete mFunctionCall;

        if (mArgument)
            delete mArgument;
    }

    void accept(Visitor *v) {
        // @note This is a workaround. When processing a mockup function we have
        // two options: 1. generate a function body for a declaration or 2. change
        // the return value of a function which already has a function body.
        // Thus we don't want to generate code for a function call from a
        // MockupFunction. This workaround might need to be changed if there
        // is a Visitor which needs to visit a FunctionCall before or after a
        // MockupFunction
        // mFunctionCall->accept(v);
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

    void useMockupFunction() {
        mOriginalFunction->replaceAllUsesWith(mMockupFunction);
    }

    void useOriginalFunction(llvm::ExecutionEngine *ee) {
//        ee->freeMachineCodeForFunction(mOriginalFunction);
        mMockupFunction->replaceAllUsesWith(mOriginalFunction);
        mMockupFunction->dump();
//        ee->freeMachineCodeForFunction(mMockupFunction);
//        ee->recompileAndRelinkFunction(mOriginalFunction);
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

    void accept(Visitor *v) {
        for (auto*& ptr : mMockupVariables)
            ptr->accept(v);

        for (auto*& ptr : mMockupFunctions)
            ptr->accept(v);

        v->VisitMockupFixture(this);
    }

    void useMockupFunction() {
        for(MockupFunction* f : mMockupFunctions)
            f->useMockupFunction();
    }

    void useOriginalFunction(llvm::ExecutionEngine *ee) {
        for(MockupFunction* f : mMockupFunctions)
            f->useOriginalFunction(ee);
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

    void accept(Visitor *v) {
        mMockupFixture->accept(v);
        v->VisitTestMockup(this);
    }

    void useMockupFunction() {
        mMockupFixture->useMockupFunction();
    }

    void useOriginalFunction(llvm::ExecutionEngine *ee) {
        mMockupFixture->useOriginalFunction(ee);
    }
};

class TestFixture : public TestExpr {
private:
    vector<TestExpr*> mStmt;
public:

    TestFixture(const vector<TestExpr*>& stmnt) : mStmt(stmnt) { }
    /// @warning @bug Watch for possible bugs here. We may need tod convert
    /// to that derived type first in order to copy!
    TestFixture(const TestFixture& that) {
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
    TestFixture* mTestFixtureExpr;
public:

    TestSetup(TestFixture* fixture) : mTestFixtureExpr(fixture) {
    }
    TestSetup(const TestSetup& that) : mTestFixtureExpr(nullptr) {
    	if (that.mTestFixtureExpr)
    		mTestFixtureExpr = new TestFixture(*that.mTestFixtureExpr);
    }

    ~TestSetup() {
        if (mTestFixtureExpr) delete mTestFixtureExpr;
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
    TestFunction(const TestFunction& that)
    : mFunctionCall(nullptr), mExpectedResult(nullptr) {
    	if(that.mFunctionCall)
    		mFunctionCall = new FunctionCall(*that.mFunctionCall);
    	if(that.mExpectedResult)
    		mExpectedResult = new ExpectedResult(*that.mExpectedResult);
    }

    ~TestFunction() {
        if(mFunctionCall) delete mFunctionCall;
        if(mExpectedResult) delete mExpectedResult;
    }

    FunctionCall* getFunctionCall() const {return mFunctionCall; }
    ExpectedResult* getExpectedResult() const {return mExpectedResult; }

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
    TestFixture* mTestFixture;
public:

    TestTeardown(TestFixture* fixture) : mTestFixture(fixture) {
    }

    ~TestTeardown() {
        if (mTestFixture) delete mTestFixture;
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
    TestData(const TestData& that) : mDataPath(nullptr) {
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
    TestData *mTestData;
    TestFunction *FunctionCall;
    TestSetup *mTestSetup;
    TestTeardown *mTestTeardown;
    TestMockup *mTestMockup;
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
    mTestData(info), FunctionCall(function), mTestSetup(setup),
    mTestTeardown(teardown), mTestMockup(mockup)  {
    }

    TestDefinition(const TestDefinition& that)
    : mTestData(nullptr), FunctionCall(nullptr), mTestSetup(nullptr),
      mTestTeardown(nullptr), mTestMockup(nullptr), mFailedEE(that.mFailedEE) {
    	if(that.mTestData)
    		mTestData = new TestData(*that.mTestData);
    	if(that.FunctionCall)
    		FunctionCall = new TestFunction(*that.FunctionCall);
    	if(that.mTestSetup)
    		mTestSetup = new TestSetup(*that.mTestSetup);
    	if(that.mTestTeardown)
    		mTestTeardown = new TestTeardown(*that.mTestTeardown);
    	if(that.mTestMockup)
    		mTestMockup = new TestMockup(*that.mTestMockup);
    }

    TestData* getTestData() const { return mTestData; }
    TestFunction * getTestFunction() const {return FunctionCall;}
    TestMockup* getTestMockup() const { return mTestMockup; }

    virtual ~TestDefinition() {
        if (mTestData) delete mTestData;
        if (FunctionCall) delete FunctionCall;
        if (mTestSetup) delete mTestSetup;
        if (mTestTeardown) delete mTestTeardown;
        if (mTestMockup) delete mTestMockup;
    }

    void accept(Visitor *v) {
        v->VisitTestDefinitionFirst(this);
        if(mTestData) mTestData->accept(v);
        if(mTestMockup) mTestMockup->accept(v);
        if(mTestSetup) mTestSetup->accept(v);
        FunctionCall->accept(v);
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
    MockupFixture *mMockupFixture;
public:

    GlobalMockup(MockupFixture *fixt) : mMockupFixture(fixt) {    }

    ~GlobalMockup() {
        if (mMockupFixture) delete mMockupFixture;
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

    void accept(Visitor *v) {
        v->VisitGlobalTeardownFirst(this);
        // This a workaround because we can build a GlobalTeardown with or without
        // a TestFixture
        if (mTestFixture) mTestFixture->accept(v);
        v->VisitGlobalTeardown(this);
    }
};

class TestGroup : public TestExpr, public LLVMFunctionHolder {
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
        /// @todo Enable GlobalMockup
        // if (GlobalMockup) GlobalMockup->setGroupName(mName->getIdentifierStr());
        if (mGlobalSetup) mGlobalSetup->setGroupName(mName->getIdentifierStr());
        if (mGlobalTeardown) mGlobalTeardown->setGroupName(mName->getIdentifierStr());
    }
    ~TestGroup() {
        if (mName) delete mName;
        if (mGlobalMockup) delete mGlobalMockup;
        if (mGlobalSetup) delete mGlobalSetup;
        if (mGlobalTeardown) delete mGlobalTeardown;
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

    string getGroupName() const { return mName->getIdentifierStr(); }
    GlobalSetup* getGlobalSetup() const { return mGlobalSetup; }
    GlobalTeardown* getGlobalTeardown() const { return mGlobalTeardown; }
    GlobalMockup* getGlobalMockup() const { return mGlobalMockup; }
    void setGlobalTeardown(GlobalTeardown* gt) { mGlobalTeardown = gt; }
    // Use it with wisdom, you can modify the internal object structure.
    vector<TestExpr*>& getTests() { return mTests; }
};

class TestFile : public TestExpr {
private:
    TestGroup* mTestGroups;
public:

    TestFile(TestGroup *tg) :
    mTestGroups(tg) {

    }

    ~TestFile() {
        if (mTestGroups) delete mTestGroups;
    }

    void accept(Visitor *v) {
        v->VisitTestFileFirst(this);
        mTestGroups->accept(v);
        v->VisitTestFile(this);
    }
};

class FunctionArgument : public TestExpr{
protected:
    Argument *argArgument;
    BufferAlloc *argBuffAlloc;
    unique_ptr<DataPlaceholder> mDP;
    unsigned    ArgIndx;
    // This parent is only used for the TestGeneratorVisitor
    FunctionCall *Parent;// Pointer to its parent
public:
    explicit FunctionArgument(Argument *arg) :
        argArgument(arg), argBuffAlloc(nullptr), mDP(nullptr), ArgIndx(0), Parent(nullptr) { }
    explicit FunctionArgument(BufferAlloc *arg) :
        argArgument(nullptr), argBuffAlloc(arg), mDP(nullptr),ArgIndx(0), Parent(nullptr) { }
    explicit FunctionArgument(unique_ptr<DataPlaceholder> dp) :
            argArgument(nullptr), argBuffAlloc(nullptr), mDP(move(dp)), ArgIndx(0), Parent(nullptr) { }
    FunctionArgument(const FunctionArgument& that):
    	argArgument(nullptr), argBuffAlloc(nullptr), mDP(nullptr), ArgIndx(0), Parent(nullptr) {
    	if(that.argArgument)
    		argArgument = new Argument(*that.argArgument);
    	if(that.argBuffAlloc)
    		argBuffAlloc = new BufferAlloc(*that.argBuffAlloc);
    	if(that.mDP.get())
    		mDP = unique_ptr<DataPlaceholder>(new DataPlaceholder(*that.mDP.get()));
    	ArgIndx = that.ArgIndx;
    	// @bug what about the pointer to the parent? Check when we should set it.
    }
    ~FunctionArgument() {
        if(argArgument)
            delete argArgument;
        if(argBuffAlloc)
            delete argBuffAlloc;
    }

    bool isDataPlaceholder() const { return mDP.get() != nullptr; }
    TokenType getTokenType() const {
        if(argArgument) return argArgument->getTokenType();
        return TOK_BUFF_ALLOC;
    }

    string toString() {
        if(argArgument)
            return argArgument->getStringRepresentation();
        if(mDP.get())
        	return "@";
        return "BufferAlloc";
    }
    unsigned getIndex() const { return ArgIndx; }
    void setIndex(unsigned ndx) {ArgIndx = ndx; }
    const string& getStringRepresentation() const {
        return argArgument->getStringRepresentation();
    }

    BufferAlloc* getBufferAlloc() const { return argBuffAlloc; }

    void accept(Visitor *v) {
        if(argArgument)
            argArgument->accept(v);
        if(argBuffAlloc)
            argBuffAlloc->accept(v);

        v->VisitFunctionArgument(this);// TODO IMplement
    }

        const Argument* getArgument() const {
            return argArgument;
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

    ~TestDriver() { }

    TestExpr* ParseTestExpr();

protected:
    Tokenizer mTokenizer;
	Token mCurrentToken;

    /// Generates default names for groups
    Identifier* groupNameFactory();
    unique_ptr<DataPlaceholder> ParseDataPlaceholder();
    Identifier* ParseIdentifier();
    Argument* ParseArgument();
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
    GlobalMockup* ParseGlobalMockup();
    GlobalSetup* ParseGlobalSetup();
    GlobalTeardown* ParseGlobalTeardown();
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
