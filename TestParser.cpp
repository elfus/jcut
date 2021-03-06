//===-- jcut/TestParser.cpp - Parse the test definitions --------*- C++ -*-===//
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
#include "JCUTScanner.h"
#include "TestParser.h"
#include <iostream>
#include <exception>
#include "llvm/Support/FileSystem.h"

using namespace std;
using namespace tp;

int TestGroup::group_count = 0;
string JCUTException::mExceptionSource;

extern unsigned column;
extern "C" int yylex();


UnexpectedToken::UnexpectedToken(tp::Token token, string expected)
: mToken(token), mExpected(expected) {
	stringstream ss;
	if(!mExceptionSource.empty())
		ss << mExceptionSource <<":";
	ss << mToken.mLine << ":" << mToken.mColumn <<": UnexpectedToken: ";
	if(mToken.mType == TOK_EOF)
		ss << "End of file. ";
	else
		ss << mToken.mLexeme << ". ";
	if(!mExpected.empty())
		ss << "Expecting a valid " << mExpected << ".";
	mMsg = ss.str();
}

const char* UnexpectedToken::what() const throw () {

	return mMsg.c_str();
}


ostream& tp::operator << (ostream& os, Token& token)
{
    os << token.mLine << ":" << token.mColumn << ": [" << token.mType << "] " << token.mLexeme;
    return os;
}

void Tokenizer::tokenize(const string& filename)
{
	mTokens.clear();

	yyin = fopen( filename.c_str(), "r" );
	if(yyin == nullptr)
		throw JCUTException("Could not open file "+filename);
	int type = TokenType::TOK_ERR;

	while((type = yylex()) != TOK_EOF) {
		mTokens.push_back(std::move(Token(yytext, yyleng, type, yylineno, column)));
	}
	fclose(yyin);

	mTokens.push_back(std::move(Token(yytext, yyleng, type, yylineno, column+1)));
	mNextToken = mTokens.begin();
}

void Tokenizer::tokenize(const char* buffer)
{
	mTokens.clear();

	YY_BUFFER_STATE state = yy_scan_string(buffer);

	int type = TokenType::TOK_ERR;

	while((type = yylex()) != TOK_EOF) {
		mTokens.push_back(std::move(Token(yytext, yyleng, type, yylineno, column)));
	}

	mTokens.push_back(std::move(Token(yytext, yyleng, type, yylineno, column+1)));
	mNextToken = mTokens.begin();

	yy_delete_buffer(state);
}

Token Tokenizer::peekToken()
{
	return *mNextToken;
}

Token Tokenizer::nextToken()
{
	Token next = *mNextToken;
	mNextToken++;
	return next;
}

//===-----------------------------------------------------------===//
//
// TestParser method definition
//

int TestExpr::leaks = 0;
unsigned LLVMFunctionHolder::warning_count = 0;

unique_ptr<DataPlaceholder> TestDriver::ParseDataPlaceholder()
{
	if (mCurrentToken != '@')
		throw UnexpectedToken(mCurrentToken, "DataPlaceHolder '@'");

	unique_ptr<DataPlaceholder> pl(new DataPlaceholder);
	mCurrentToken = mTokenizer.nextToken();
	return pl;
}

BufferAlloc* TestDriver::ParseBufferAlloc()
{
	if (mCurrentToken != '[')
		throw UnexpectedToken(mCurrentToken, "left sqared bracket '[' for BufferAllocation");

	mCurrentToken = mTokenizer.nextToken(); // eat up the '['

	if (mCurrentToken != TOK_INT)
		throw UnexpectedToken(mCurrentToken, "int constant for buffer size");

	NumericConstant* buff_size = ParseNumericConstant();// Buffer Size

	BufferAlloc *ba = nullptr;
	if (mCurrentToken == ':') {
		mCurrentToken = mTokenizer.nextToken(); // eat up the ':'
		if (mCurrentToken == '{') { // indicates a StructInitializer
			StructInitializer* struct_init = ParseStructInitializer();
			ba = new BufferAlloc(buff_size, struct_init);
		} else if(mCurrentToken == TOK_INT || mCurrentToken == TOK_FLOAT) {
			NumericConstant* default_val = ParseNumericConstant();
			ba = new BufferAlloc(buff_size, default_val);
		} else
			throw UnexpectedToken(mCurrentToken, " int, float or struct initializer.");

		mCurrentToken = mTokenizer.nextToken(); // eat up the ']'
		return ba;
	}
	mCurrentToken = mTokenizer.nextToken(); // eat up the ']'
	ba = new BufferAlloc(buff_size);// default value to 0
	return ba;
}

Identifier* TestDriver::ParseIdentifier()
{
	if (mCurrentToken != TOK_IDENTIFIER)
		throw UnexpectedToken(mCurrentToken, "identifier");
	Identifier * id = new Identifier(mCurrentToken.mLexeme);
	mCurrentToken = mTokenizer.nextToken(); // eat current identifier
	return id;
}

FunctionArgument* TestDriver::ParseFunctionArgument()
{
	if (mCurrentToken == '[') { // Buffer allocation
		return new FunctionArgument(ParseBufferAlloc());
	}
	else
	if (mCurrentToken == '@') { // DataPlaceholder
		unique_ptr<DataPlaceholder> pl = ParseDataPlaceholder();
		return new FunctionArgument(move(pl));
	}
	else
		return new FunctionArgument(ParseConstant());
	throw UnexpectedToken(mCurrentToken,"function argument");
}

InitializerValue* TestDriver::ParseInitializerValue()
{
	if (mCurrentToken == '{') {
		StructInitializer* val = ParseStructInitializer();
		return new InitializerValue(val);
	}
	else {
		NumericConstant* arg = ParseNumericConstant();
		return new InitializerValue(arg);
	}
}

DesignatedInitializer* TestDriver::ParseDesignatedInitializer()
{
	Identifier* id = nullptr;
	InitializerValue* arg = nullptr;
	vector<tuple<Identifier*,InitializerValue*>> init;
	do {
		if (mCurrentToken == ',')
			mCurrentToken = mTokenizer.nextToken(); // eat up the ','

		if (mCurrentToken != '.') {
			for(tuple<Identifier*,InitializerValue*>& tup : init) {
				id = get<0>(tup);
				arg = get<1>(tup);
				delete id;
				delete arg;
			}
			throw UnexpectedToken(mCurrentToken);
		}

		mCurrentToken = mTokenizer.nextToken(); // eat up the '.'
		id = ParseIdentifier();
		if (mCurrentToken != '=') {
                    for(tuple<Identifier*,InitializerValue*>& tup : init) {
                            id = get<0>(tup);
                            arg = get<1>(tup);
                            delete id;
                            delete arg;
                    }
			delete id;
			throw UnexpectedToken(mCurrentToken);
		}
		mCurrentToken = mTokenizer.nextToken(); // eat up the '='

		arg = ParseInitializerValue();

		init.push_back(make_tuple(id,arg));
	} while(mCurrentToken == ',');

	DesignatedInitializer* di = new DesignatedInitializer(init);
	return di;
}

InitializerList* TestDriver::ParseInitializerList()
{
	InitializerValue* arg = nullptr;
	vector<InitializerValue*> init;
	do {
		if (mCurrentToken == ',')
			mCurrentToken = mTokenizer.nextToken();
		arg = ParseInitializerValue();
		init.push_back(arg);
	} while(mCurrentToken == ',');

	InitializerList* il = new InitializerList(init);
	return il;
}

StructInitializer* TestDriver::ParseStructInitializer()
{
	if (mCurrentToken != '{')
		throw UnexpectedToken(mCurrentToken,"struct initializer");
	mCurrentToken = mTokenizer.nextToken(); // eat up the '{'

	StructInitializer* si = nullptr;
	InitializerList* il = nullptr;
	if (mCurrentToken == '.') {
		DesignatedInitializer* di = ParseDesignatedInitializer();
		si = new StructInitializer(di);
	} else {
		il = ParseInitializerList();
		si = new StructInitializer(il);
	}

	if(mCurrentToken != '}') {
		if(si) delete si;
                if(il) delete il;
		throw UnexpectedToken(mCurrentToken, "right curly bracket '}'");
	}

	mCurrentToken = mTokenizer.nextToken(); // eat up the '}'
	return si;
}

VariableAssignment* TestDriver::ParseVariableAssignment()
{
	Identifier *identifier = ParseIdentifier();

	if (mCurrentToken != '=') {
		delete identifier;
		throw UnexpectedToken(mCurrentToken, "assignment operator '='");
	}
	mCurrentToken = mTokenizer.nextToken(); // eat up the '='

	if (mCurrentToken == '{') {
		StructInitializer* struct_init = ParseStructInitializer();
		return new VariableAssignment(identifier, struct_init);
	}

	if (mCurrentToken == '[') {
		BufferAlloc* ba = ParseBufferAlloc();
		return new VariableAssignment(identifier, ba);
	}

	Constant *constant = ParseConstant();
	return new VariableAssignment(identifier, constant);
}

Identifier* TestDriver::ParseFunctionName()
{
	if (mCurrentToken != TOK_IDENTIFIER)
		throw UnexpectedToken(mCurrentToken, "function name");
	return ParseIdentifier(); // Missing a 'FunctionName' class that wraps an Identifier
}

FunctionCall* TestDriver::ParseFunctionCall()
{
	Identifier* functionName = ParseFunctionName();

	if (mCurrentToken != '(') {
            delete functionName;
            throw UnexpectedToken(mCurrentToken, "left parenthesis '(' for function call");
        }

	mCurrentToken = mTokenizer.nextToken(); // eat the (
	vector<FunctionArgument*> Args;
	while (mCurrentToken != ')') {
		FunctionArgument *arg = ParseFunctionArgument();
		Args.push_back(arg);

		if (mCurrentToken == ')')
			break;

		if (mCurrentToken != ',') {
			for (auto*& ptr : Args)
				delete ptr;
                        delete functionName;
			throw UnexpectedToken(mCurrentToken,"comma or right parenthesis inside function parameters");
		}

		mCurrentToken = mTokenizer.nextToken(); // eat the ','
	}

	mCurrentToken = mTokenizer.nextToken(); // Eat the )

	return new FunctionCall(functionName, Args);
}

ExpectedResult* TestDriver::ParseExpectedResult()
{
	ComparisonOperator* cmp = ParseComparisonOperator();
        ExpectedConstant* EC = ParseExpectedConstant();
	return new ExpectedResult(cmp, EC);
}

ComparisonOperator* TestDriver::ParseComparisonOperator()
{
	if (mCurrentToken == TOK_COMPARISON_OP) {
		ComparisonOperator* cmp = new ComparisonOperator(mCurrentToken.mLexeme);
		mCurrentToken = mTokenizer.nextToken(); // consume TOK_COMPARISON_OP
		return cmp;
	}
	throw UnexpectedToken(mCurrentToken, "ComparisonOperator: ==, !=, >=, <=, <, or >");
}

ExpectedConstant* TestDriver::ParseExpectedConstant()
{
	if (mCurrentToken == '@') {
		unique_ptr<DataPlaceholder> pl = ParseDataPlaceholder();
		return new ExpectedConstant(move(pl));
	}
    return new ExpectedConstant(ParseConstant());
}

StringConstant* TestDriver::ParseStringConstant()
{
	if(mCurrentToken == TOK_STRING) {
		StringConstant* sc = new StringConstant(mCurrentToken.mLexeme);
		mCurrentToken = mTokenizer.nextToken(); // Consume the constant
		return sc;
	}
	throw UnexpectedToken(mCurrentToken, "string constant: \"example\"");
}

CharConstant* TestDriver::ParseCharConstant()
{
	CharConstant* cc = nullptr;
	// Watch for the index 1, the flex parser stores the char constant with
	// single quotes.
	// @bug What happens when we receive an escaped character?
	cc = new CharConstant(mCurrentToken.mLexeme[1]);
	mCurrentToken = mTokenizer.nextToken(); // Consume the constant
	return cc;
}

NumericConstant* TestDriver::ParseNumericConstant()
{
	if(mCurrentToken != TOK_INT && mCurrentToken != TOK_FLOAT)
		throw UnexpectedToken(mCurrentToken,"numeric constant (int or float)");
	NumericConstant* nc = nullptr;
	if (mCurrentToken == TOK_INT)
		nc = new NumericConstant(atoi(mCurrentToken.mLexeme.c_str()));
	else
	if(mCurrentToken == TOK_FLOAT)
		nc = new NumericConstant((float)atof(mCurrentToken.mLexeme.c_str()));
	mCurrentToken = mTokenizer.nextToken(); // Consume the constant
	return nc;
}

Constant* TestDriver::ParseConstant()
{
	Constant* C = nullptr;
	Token token = mCurrentToken;
	if (mCurrentToken == TOK_INT || mCurrentToken == TOK_FLOAT)
		C = new Constant(ParseNumericConstant());
	else
    if(mCurrentToken == TOK_STRING)
		C = new Constant(ParseStringConstant());
	else
    if(mCurrentToken == TOK_CHAR)
		C = new Constant(ParseCharConstant());
	else
		throw UnexpectedToken(mCurrentToken,"constant (string, char, int or float)");
	// This is a workaround for float and double values. Let LLVM do the work
	// on what type of precision to choose, we will just pass a string representing
	// the floating point value.
	C->setString(token.mLexeme);

	return C;
}

TestTeardown* TestDriver::ParseTestTearDown()
{
	if (mCurrentToken != TOK_AFTER)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'after'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		TestFixture* testFixture = ParseTestFixture();
		mCurrentToken = mTokenizer.nextToken(); //eat the  '}'
		return new TestTeardown(testFixture);
	}
	throw UnexpectedToken(mCurrentToken,"left curly bracket '{' after keyword 'after'");
}

TestFunction* TestDriver::ParseTestFunction()
{
	FunctionCall *FunctionCall = ParseFunctionCall();
	ExpectedResult* ER = nullptr;
	if (mCurrentToken == TOK_COMPARISON_OP)
		ER = ParseExpectedResult();
	if (mCurrentToken != ';') {
            delete FunctionCall;
            if(ER) delete ER;
            throw UnexpectedToken(mCurrentToken,"semicolon ';' after test expression");
        }
	mCurrentToken = mTokenizer.nextToken(); //eat up the ';'
	return new TestFunction(FunctionCall, ER);
}

TestSetup* TestDriver::ParseTestSetup()
{
	if (mCurrentToken != TOK_BEFORE)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'before'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		TestFixture* testFixture = ParseTestFixture();
		mCurrentToken = mTokenizer.nextToken(); //eat the  '}'
		return new TestSetup(testFixture);
	}
	throw UnexpectedToken(mCurrentToken,"left curly bracket '{' after keyword 'before'");
}

TestFixture* TestDriver::ParseTestFixture()
{
	vector<TestExpr*> statements;
	FunctionCall *func = nullptr;
	VariableAssignment* var = nullptr;
	ExpectedExpression* exp = nullptr;

	while (mCurrentToken != '}') {
		if (mTokenizer.peekToken() == '(') { // this is a function
			func = (FunctionCall*) ParseFunctionCall();
			statements.push_back(func);
			func = nullptr;
		} else if (mTokenizer.peekToken() == TOK_COMPARISON_OP) {
			exp = ParseExpectedExpression();
			statements.push_back(exp);
			exp = nullptr;
		} else if(mTokenizer.peekToken() == '=') {
			var = (VariableAssignment*) ParseVariableAssignment();
			statements.push_back(var);
			var = nullptr;
		}

		if (mCurrentToken != ';') {
			for(auto*& ptr : statements) delete ptr;
			throw UnexpectedToken(mCurrentToken,"semicolon ';' inside test fixture");
		}

		if (mCurrentToken == ';')
			mCurrentToken = mTokenizer.nextToken(); // eat up the ';'

		if (mCurrentToken == '}')
			break;

	}
	return new TestFixture(statements);
}

ExpectedExpression* TestDriver::ParseExpectedExpression()
{
	// In case the ExpectedExpression is well formed, this token represents
	// the position where it starts.
	int line = mCurrentToken.mLine;
	int column = mCurrentToken.mColumn;
	Operand* LHS = ParseOperand();
	ComparisonOperator* CO = ParseComparisonOperator();
	Operand* RHS = ParseOperand();
	return new ExpectedExpression(LHS, CO, RHS, line, column);
}

Operand* TestDriver::ParseOperand()
{
	if(mCurrentToken == TOK_IDENTIFIER) {
		Identifier* I = ParseIdentifier();
		return new Operand(I);
	} else {
		Constant* C = ParseConstant();
		return new Operand(C);
	}
	throw UnexpectedToken(mCurrentToken,"identifier or constant(string, char, int or float)");
}

MockupVariable* TestDriver::ParseMockupVariable()
{
	VariableAssignment *varAssign = ParseVariableAssignment();
	return new MockupVariable(varAssign);
}

MockupFunction* TestDriver::ParseMockupFunction()
{
	FunctionCall *func = ParseFunctionCall();

	if (mCurrentToken != '=') {
		delete func;
		throw UnexpectedToken(mCurrentToken,"assignment operator '=' for mockup function");
	}
	mCurrentToken = mTokenizer.nextToken(); // eat the '='

	if(mCurrentToken == "void") {
		Identifier* id = ParseIdentifier();
		return new MockupFunction(func, id);
	}
	else {
		Constant *expected_const = ParseConstant();
		return new MockupFunction(func, expected_const);
	}
	return nullptr;
}

MockupFixture* TestDriver::ParseMockupFixture()
{
	vector<MockupFunction*> MockupFunctions;
	vector<MockupVariable*> MockupVariables;

	while (mCurrentToken != '}') {

		MockupFunction *func = nullptr;
		MockupVariable *var = nullptr;
		if (mTokenizer.peekToken() == '(') {
			func = (MockupFunction*) ParseMockupFunction();
			MockupFunctions.push_back(func);
		} else if (mTokenizer.peekToken() == '=') {
			var = (MockupVariable*) ParseMockupVariable();
			MockupVariables.push_back(var);
		}

		if (mCurrentToken == '}')
			break;

		if (mCurrentToken != ';') {
			for(auto*& ptr : MockupFunctions) delete ptr;
			for(auto*& ptr : MockupVariables) delete ptr;
            throw UnexpectedToken(mCurrentToken,"semicolon ';' after mockup function");
		}
		mCurrentToken = mTokenizer.nextToken(); //eat up the ';'
	}
	return new MockupFixture(MockupFunctions, MockupVariables);
}

TestMockup* TestDriver::ParseTestMockup()
{
	if (mCurrentToken != TOK_MOCKUP)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'mockup'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		MockupFixture* mf = (MockupFixture*) ParseMockupFixture();
		mCurrentToken = mTokenizer.nextToken(); // eat the '}'
		return new TestMockup(mf);
	}
	throw UnexpectedToken(mCurrentToken,"left curly bracket '{'");
}

TestData* TestDriver::ParseTestData()
{
	if (mCurrentToken != TOK_TEST_DATA) {
		// @todo Create default information
		return nullptr;
	}
	mCurrentToken = mTokenizer.nextToken(); // eat the keyword data
	if (mCurrentToken != '{')
		throw UnexpectedToken(mCurrentToken, "left curly bracket '{'");

	mCurrentToken = mTokenizer.nextToken(); // eat up the {

	unique_ptr<StringConstant> name = unique_ptr<StringConstant>(ParseStringConstant());

	if (mCurrentToken != ';')
		throw UnexpectedToken(mCurrentToken,"semicolon ';'");

	mCurrentToken = mTokenizer.nextToken(); // eat up the }

	if (mCurrentToken != '}')
		throw UnexpectedToken(mCurrentToken,"right curly bracket '}'");

	mCurrentToken = mTokenizer.nextToken(); // eat up the }

	return new TestData(move(name));
}

TestDefinition* TestDriver::ParseTestDefinition()
{
	TestData *info = ParseTestData();
	TestMockup *mockup = ParseTestMockup();
	TestSetup *setup = ParseTestSetup();
	TestFunction *testFunction = ParseTestFunction();
	TestTeardown *teardown = ParseTestTearDown();

	return new TestDefinition(info, testFunction, setup, teardown, mockup);
}

TestGroup* TestDriver::ParseTestGroup(Identifier* name)
{
	GlobalMockup *gm = nullptr;
	GlobalSetup *gs = nullptr;
	GlobalTeardown *gt = nullptr;
	vector<TestExpr*> tests;
	Identifier* new_group_name = nullptr;

	try {
		gm = ParseGroupMockup();
		gs = ParseGroupSetup();

		if(mCurrentToken == TOK_AFTER_ALL)
			throw UnexpectedToken(mCurrentToken,"test definition");

		///////////////////////////////////////
		// Parse all the tests
		//
		while (true) {
			if (mCurrentToken == '}' or mCurrentToken == TOK_EOF or
					mCurrentToken == TOK_AFTER_ALL)
				break;

			if (mCurrentToken == TOK_GROUP) {

				mCurrentToken = mTokenizer.nextToken(); // eat up the group keyword

				if (mCurrentToken == TOK_IDENTIFIER)
					new_group_name = ParseIdentifier();

				if(new_group_name == nullptr)
					new_group_name = groupNameFactory();

				if (mCurrentToken != '{') {
					delete new_group_name;
					throw UnexpectedToken(mCurrentToken,"left curly bracket '{' for this group");
				}

				mCurrentToken = mTokenizer.nextToken();// eat up the '{'
				Identifier* ngn = new Identifier(name->toString()+":"+new_group_name->toString());
				delete new_group_name;
				TestGroup* group = ParseTestGroup(ngn);
				if (group)
					tests.push_back(group);

				if(mCurrentToken != '}') {
					delete ngn;
					delete group;
					throw UnexpectedToken(mCurrentToken,"right curly bracket '}' for this group");
				}
				mCurrentToken = mTokenizer.nextToken(); // eat up the character '}'
			} else {
				TestDefinition *testDefinition = ParseTestDefinition();
				if(name)
					testDefinition->setGroupName(name->toString());
				else
					testDefinition->setGroupName("default");

				tests.push_back(testDefinition);
			}
		}

		////////////////////////////////////////
		// Changed the syntax and parse the groups teardown at the very end of the
		// group
		gt = ParseGroupTeardown();
	} catch(...) {
		if(gm) delete gm;
		if(gs) delete gs;

		for(auto*& ptr : tests)
			delete ptr;

		tests.clear();
		throw;// Just cleanup before leaving current group
	}

	return new TestGroup(name, tests, gm, gs, gt);
}

GlobalMockup* TestDriver::ParseGroupMockup()
{
	if (mCurrentToken == TOK_MOCKUP_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			MockupFixture *mf = (MockupFixture*) ParseMockupFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalMockup(mf);
		}
		throw UnexpectedToken(mCurrentToken,"left curly bracket '{' after mockup_all");
	}
	return nullptr;
}

GlobalSetup* TestDriver::ParseGroupSetup()
{
	if (mCurrentToken == TOK_BEFORE_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixture *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalSetup(mf);
		}
		throw UnexpectedToken(mCurrentToken,"left curly bracket '{' after before_all");
	}
	return nullptr;
}

GlobalTeardown* TestDriver::ParseGroupTeardown()
{
	if (mCurrentToken == TOK_AFTER_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixture *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalTeardown(mf);
		}
		throw UnexpectedToken(mCurrentToken,"left curly bracket '{' after after_all");
	}
	return nullptr;
}

TestFile* TestDriver::ParseTestFile()
{
	TestGroup* group = ParseTestGroup(groupNameFactory());
	if (group)
		return new TestFile(group);
	else {
            delete group;
            throw JCUTException("Invalid test file");
        }
}

TestExpr* TestDriver::ParseTestExpr()
{
	// Position on the first token
	mCurrentToken = mTokenizer.nextToken();
	return ParseTestFile();
}

Identifier* TestDriver::groupNameFactory()
{
	static unsigned i = 0;
	stringstream ss;
	ss << "group_" << i;
	i++;
	return new Identifier(ss.str());
}

// Definitions due to conflicts with the forward declarations
FunctionCall::FunctionCall(Identifier* name, const vector<FunctionArgument*>& arg) :
mFunctionName(name), mFunctionArguments(arg), mReturnType(nullptr)
{
	TestExpr::type = TestExpr::FUNC_CALL;
	unsigned i = 0;
	for (auto*& arg : mFunctionArguments) {
		arg->setIndex(i);
		++i;
	}
}

FunctionCall::~FunctionCall()
{
	for (FunctionArgument*& ptr : mFunctionArguments) {
		delete ptr;
		ptr = nullptr;
	}
}

void FunctionCall::accept(Visitor *v)
{
	v->VisitFunctionCallFirst(this);
	for (FunctionArgument*& ptr : mFunctionArguments) {
		ptr->accept(v);
	}
	v->VisitFunctionCall(this);
}

void InitializerValue::accept(Visitor *v) {
	if (mNC) mNC->accept(v);
	if (mStructValue) mStructValue->accept(v);
	v->VisitInitializerValue(this);
}


string FunctionCall::getFunctionCalledString() {
	string called = mFunctionName->toString() +"(";
	// @todo @bug We have to improve the way we detect and store the arguments.
	// For instance when calling a function that receives a char, we need to
	// print a character and not a number, and when calling a function that
	// receives a numeric constant or integral type we have to display number.
	if(mFunctionArguments.size()) {
		for(FunctionArgument* fa : mFunctionArguments)
			called += fa->toString() + ", ";
		called.pop_back();
		called.pop_back();
	}
	called += ")";
	return called;
}

bool FunctionCall::hasDataPlaceholders() const
{
	for(auto a : mFunctionArguments)
		if(a->isDataPlaceholder())
			return true;
	return false;
}

vector<unsigned> FunctionCall::getDataPlaceholdersPos() const
{
	vector<unsigned> positions;
	unsigned i = 0;
	for(auto p : mFunctionArguments) {
		if(p->isDataPlaceholder())
			positions.push_back(i);
		++i;
	}
	return positions;
}

FunctionCall::FunctionCall(const FunctionCall& that)
: TestExpr(that), mFunctionName(nullptr), mFunctionArguments(),
  mReturnType(nullptr) {
	if (that.mFunctionName)
		mFunctionName = unique_ptr<Identifier>(
				new Identifier(*that.mFunctionName));
	for(FunctionArgument* fa : that.mFunctionArguments) {
		FunctionArgument* copy = new FunctionArgument(*fa);
		mFunctionArguments.push_back(copy);
	}
}

bool FunctionCall::replaceDataPlaceholder(unsigned pos, FunctionArgument* new_arg) {
	if(pos >= mFunctionArguments.size())
		assert(false && "Invalid replacement position!");

	if(mFunctionArguments[pos]->isDataPlaceholder() == false)
		assert(false && "This is not a DataPlaceholder!");

	FunctionArgument* to_delete = mFunctionArguments[pos];
	new_arg->setIndex(to_delete->getIndex());
	mFunctionArguments[pos] = new_arg;
	delete to_delete;
	to_delete = nullptr;
	return true;
}

InitializerValue::InitializerValue(const InitializerValue& that)
: TestExpr(that), mNC(nullptr), mStructValue(nullptr) {
	if (that.mNC)
		mNC = unique_ptr<NumericConstant>(new NumericConstant(*that.mNC));
	if (that.mStructValue)
		mStructValue = unique_ptr<StructInitializer>(
				new StructInitializer(*that.mStructValue));
}

///////
// CSVDriver
CSVDriver::CSVDriver(const string& filename) : TestDriver(), rows(0), columns(0)
{
	ifstream csv(filename);
	if(!csv)
		throw JCUTException("CSV File: "+filename+" not found!");
	string line;
	getline(csv, line);
	vector<string> items = split(line,',');
	columns = items.size();
	mMatrix.push_back(items);

	while(getline(csv, line)) {
		items = split(line, ',');
		if(items.empty()) continue;
		if(items.size() != columns)
			throw JCUTException("Column mismatch in CSV file: "+filename);

		mMatrix.push_back(items);
	}
	rows = mMatrix.size();
}

vector<string> CSVDriver::split(string line, char split_char)
{
	vector<string> splitted;
	string item;
	bool struct_init = false;
	for(auto c : line) {
		// quick workaround to detect commas inside a struct
		// initialization and ignore them
		if(c == '{')
			struct_init = true;
		if(c == '}')
			struct_init = false;
		if(c == split_char && !struct_init) {
			if(item.size())
				splitted.push_back(item);
			item = "";
			continue;
		}
		item += c;
	}
	if(item.size())
		splitted.push_back(item);
	return splitted;
}

unsigned CSVDriver::columnCount()
{
	return columns;
}

unsigned CSVDriver::rowCount()
{
	return rows;
}

FunctionArgument* CSVDriver::getFunctionArgumentAt(unsigned i, unsigned j)
{
	string str = mMatrix[i][j];
	mTokenizer.tokenize(str.c_str());
	mCurrentToken = mTokenizer.nextToken();
	if(mCurrentToken == '@')
		assert(false && "DataPlaceholders not supported in CVS file.");

	return ParseFunctionArgument();
}

ExpectedConstant* CSVDriver::getExpectedConstantAt(unsigned i, unsigned j)
{
	string str = mMatrix[i][j];
	mTokenizer.tokenize(str.c_str());
	mCurrentToken = mTokenizer.nextToken();
	if(mCurrentToken == '@')
			assert(false && "DataPlaceholders not supported in CVS file.");
	return ParseExpectedConstant();
}

///////////////////
void TestResults::saveToDisk() {
	if(!using_fork)
		return;

	if(mPipe[PWRITE] == 0)
		throw JCUTException("Invalid pipe for WRITING results!");
	close(mPipe[PREAD]); // Child writes, never reads

	stringstream ss;
	for(auto it = mResults.begin(); it != mResults.end(); ++it) {
		ss << "[" << mColumnNames[it->first] << "]" << endl;
		ss << it->second << endl;
	}

	string data = ss.str();
	data += '\0';
	int rc = write(mPipe[PWRITE], data.c_str(), static_cast<size_t>(data.size()));
	if(rc == -1)
		throw JCUTException("Error while sending data to the parent process");
	if(rc == 0)
		cout << "Warning: no data was sent to the parent process";
	close(mPipe[PWRITE]); /* Reader will see EOF */
}

ColumnName TestResults::getColumnFromStr(const string& str)
{
	ColumnName col = MAX_COLUMN;
	for(auto it = mColumnNames.begin(); it != mColumnNames.end(); ++it) {
		if(str == it->second) {
			col = it->first;
			break;
		}
	}
	return col;
}

map<ColumnName, string> TestResults::readFromDisk()
{
	if(!using_fork)
		return mResults;

	if(mPipe[PREAD] == 0)
		throw JCUTException("Invalid pipe for READING results!");
	close(mPipe[PWRITE]); // Parent reads, never writes

	map<ColumnName, string> results;
	string line;

	ColumnName current;
	const int SIZE = 1024;
	char buf[SIZE];
	int bytes_read = 0;
	int total = 0;
	stringstream ss;
	while((bytes_read = read(mPipe[PREAD],buf, SIZE)) > 0) {
		ss << buf;
		memset(buf, 0, SIZE);
		total += bytes_read;
	}

	if (total == 0)
		throw JCUTException("The test crashed during execution!");

	while(!ss.eof()) {
		std::getline(ss, line);
		if(line.empty())
			continue;

		if(line[0] == '[') {
			string column_name = line.substr(1,line.size()-2);
			current = getColumnFromStr(column_name);
			results[current] = "";
			continue;
		}

		if(current == WARNING or current == FUD_OUTPUT or current == FAILED_EE)
			results[current] += "\n"+line;
		else
			results[current] += line;
	}

	close(mPipe[PREAD]);
	return results;
}

void TestResults::collectTestResults(tp::TestDefinition* TD)
{
	for(auto column : mOrder) {
		stringstream ss;
		mResults[column] = "";
		ss << getColumnString(column, TD);
		mResults[column] = ss.str();
	}
	// Save warnings
	const vector<Warning>& warnings = TD->getWarnings();
	if(warnings.size()) {
		stringstream ss;
		for(auto w : warnings)
			ss <<  w.what() << endl;
		mResults[WARNING] = ss.str();
	}
	// Save function under test output
	if(TD->getOutput().size()) {
		stringstream ss;
		ss << TD->getOutput();
		mResults[FUD_OUTPUT] = ss.str();
	}
	// Save Failed expected expressions
	if(TD->testPassed() == false) {
		const vector<ExpectedExpression*>&
		ee = TD->getFailedExpectedExpressions();
		for(ExpectedExpression* e : ee) {
			stringstream ss;
			ss << JCUTException::mExceptionSource << ":" << e->getLine()
				<< ":" << e->getColumn() << ": ";
			ss << "Expected expression ["
				 << e->toString() << "] is false" << endl;
			mResults[FAILED_EE] += ss.str();
		}
	}
}

string TestResults::getColumnString(ColumnName name, tp::TestDefinition *TD)
{
	switch(name) {
		case GROUP_NAME:
			return TD->getGroupName();
		case TEST_NAME:
			return TD->getLLVMFunction()->getName();
		case FUD:
			return TD->getTestFunction()->getFunctionCall()->getFunctionCalledString();
		case RESULT:
		{
			bool passed = TD->getPassingValue();
			if(TD->getFailedExpectedExpressions().size())
				passed = false;
			return (passed?"PASSED" : "FAILED");
		}
		case ACTUAL_RESULT:
			return getActualResultString(TD);
		case EXPECTED_RES:
			return getExpectedResultString(TD);
		default:
			return "Invalid column";
	}
	return "Invalid column2";
}

string TestResults::getActualResultString(TestDefinition *TD)
{
	llvm::GenericValue result(TD->getReturnValue());
	stringstream ss;
	llvm::Type *returnType = TD->getTestFunction()->getFunctionCall()->getReturnType();
	llvm::Type::TypeID type = returnType->getTypeID();
	switch(type) {
		case llvm::Type::TypeID::DoubleTyID:
			ss << result.DoubleVal;
		break;
		case llvm::Type::TypeID::FloatTyID:
			ss << result.FloatVal;
		break;
		case llvm::Type::TypeID::HalfTyID:
			ss << result.FloatVal;
			break;
		case llvm::Type::TypeID::IntegerTyID:
		{
			unsigned bit_width = result.IntVal.getBitWidth();
			bool is_signed = result.IntVal.isSignedIntN(bit_width);
			/// @note For the sake of simplicity just use 10, however we
			/// we may want to use a different radix.
			unsigned radix = 10;
			ss << result.IntVal.toString(radix,is_signed);
		}
		break;
		case llvm::Type::TypeID::PointerTyID:
			ss << result.PointerVal;
			break;
		case llvm::Type::TypeID::StructTyID:
			ss << "struct type";
			break;
		case llvm::Type::TypeID::VoidTyID:
			ss << "void";
			break;
		default:
			assert(false && "Unsupported return type");
	}
	return ss.str();
}

string  TestResults::getExpectedResultString(TestDefinition *TD)
{
	ExpectedResult* ER = TD->getTestFunction()->getExpectedResult();
	if (ER) {
		stringstream ss;
		const Constant* C = ER->getExpectedConstant()->getConstant();
		ss << ER->getComparisonOperator()->toString() << " ";
		switch(C->getType()){
			case Constant::Type::NUMERIC:
			{
				const NumericConstant* nc = C->getNumericConstant();
				if(nc->isFloat())
					ss << nc->getFloat();
				if(nc->isInt())
					ss << nc->getInt();
			}
				break;
			case Constant::Type::STRING:
			{
				const string& s = C->getStringConstant()->getString();
				stringstream tmp;
				tmp << static_cast<const void*>(s.c_str());
				ss << "0x" << std::hex << tmp.str() << " (" << s << ")";
			}
				break;
			case Constant::Type::CHAR:
			{
				unsigned u = C->getCharConstant()->getChar();
				ss << u << " ('" << C->getCharConstant()->getChar() << "')";
			}
				break;
			default:
				ss<<"Invalid Constant!";
				break;
		}
		return ss.str();
	}
	return "(none)";
}
