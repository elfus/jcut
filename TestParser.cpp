//===-- jcut/TestParser.cpp - Parse the test definitions --------*- C++ -*-===//
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
#include "JCUTScanner.h"
#include "TestParser.h"
#include <iostream>
#include <exception>

using namespace std;
using namespace tp;

int TestGroup::group_count = 0;
string Exception::mCurrentFile;

extern unsigned column;
extern "C" int yylex();

ostream& tp::operator << (ostream& os, Token& token)
{
    os << token.mLine << ":" << token.mColumn << ": [" << token.mType << "] " << token.mLexeme;
    return os;
}

Tokenizer::Tokenizer(const string& filename) : mInput(filename), mTokens(),
	mNextToken(nullptr)
{
	if (!mInput) {
		throw Exception("Could not open file " + filename);
	}

	mInput.seekg(0);

	yyin = fopen( filename.c_str(), "r" );
	int type = TokenType::TOK_ERR;
	while((type = yylex()) != TOK_EOF) {
		mTokens.push_back(std::move(Token(yytext, yyleng, type, yylineno, column)));
	}
	fclose(yyin);

	mTokens.push_back(std::move(Token(yytext, yyleng, type, yylineno, column+1)));
	mNextToken = mTokens.begin();
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

Argument* TestDriver::ParseArgument()
{
	Argument *Number = new Argument(mCurrentToken.mLexeme, mCurrentToken.mType);
	mCurrentToken = mTokenizer.nextToken(); // eat current argument, move to next
	return Number;
}

unique_ptr<DataPlaceholder> TestDriver::ParseDataPlaceholder()
{
	if (mCurrentToken != '@')
		throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
				"Expected @ (DataPlaceholder",
				"Received "+mCurrentToken.mLexeme+" instead");
	unique_ptr<DataPlaceholder> pl(new DataPlaceholder);
	mCurrentToken = mTokenizer.nextToken();
	return pl;
}

BufferAlloc* TestDriver::ParseBufferAlloc()
{
	if (mCurrentToken != '[')
		throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
			"Expected a '['BufferAlloc",
			"Received "+mCurrentToken.mLexeme+" instead.");
	mCurrentToken = mTokenizer.nextToken(); // eat up the '['

	if (mCurrentToken != TOK_INT)
		throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
			"Expected an integer constant stating the buffer size.",
			"Received "+mCurrentToken.mLexeme+" instead.");
	// @todo Create an integer class and stop using Argument
	Argument* buff_size = ParseArgument();// Buffer Size

	BufferAlloc *ba = nullptr;
	if (mCurrentToken == ':') {
		mCurrentToken = mTokenizer.nextToken(); // eat up the ':'
		if (mCurrentToken == '{') { // indicates a StructInitializer
			StructInitializer* struct_init = ParseStructInitializer();
			ba = new BufferAlloc(buff_size, struct_init);
		} else if(mCurrentToken == TOK_INT or mCurrentToken == TOK_FLOAT or
				mCurrentToken == TOK_STRING or mCurrentToken == TOK_CHAR) {
			// @todo Improve this if and they way we SHOULD handle each of those
			// tokens.
			// @todo Create an integer class and stop using Argument
			Argument* default_val = ParseArgument();
			ba = new BufferAlloc(buff_size, default_val);
		} else
			throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
				"Unexpected token: "+mCurrentToken.mLexeme);

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
		throw Exception("Expected an identifier but received " + mCurrentToken.mLexeme);
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
		return new FunctionArgument(ParseArgument());
	throw Exception("Expected a valid Argument but received: " + mCurrentToken.mLexeme);
}

InitializerValue* TestDriver::ParseInitializerValue()
{
	if (mCurrentToken == '{') {
		StructInitializer* val = ParseStructInitializer();
		return new InitializerValue(val);
	}
	else {
		Argument* arg = ParseArgument();
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
			throw Exception("Unexpected token: "+mCurrentToken.mLexeme);
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
			throw Exception("Invalid designated initializer");
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
		throw Exception("Expected struct initializer '{' but received "+mCurrentToken.mLexeme);
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
		throw Exception("Malformed designated initializer");
	}

	mCurrentToken = mTokenizer.nextToken(); // eat up the '}'
	return si;
}

VariableAssignment* TestDriver::ParseVariableAssignment()
{
	Identifier *identifier = ParseIdentifier();

	if (mCurrentToken != '=') {
		delete identifier;
		throw Exception("Expected = but received " + mCurrentToken.mLexeme);
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

	Argument *arg = ParseArgument();
	return new VariableAssignment(identifier, arg);
}

Identifier* TestDriver::ParseFunctionName()
{
	if (mCurrentToken != TOK_IDENTIFIER)
		throw Exception(mCurrentToken.mLine,mCurrentToken.mColumn,
				"Expected a valid function name.",
				"Received "+ mCurrentToken.mLexeme+" instead.");
	return ParseIdentifier(); // Missing a 'FunctionName' class that wraps an Identifier
}

FunctionCall* TestDriver::ParseFunctionCall()
{
	Identifier* functionName = ParseFunctionName();

	if (mCurrentToken != '(') {
            string extra = functionName->getIdentifierStr()+mCurrentToken.mLexeme;
            delete functionName;
            throw Exception(mCurrentToken.mLine,mCurrentToken.mColumn,
                            "Expected a left parenthesis in function call but received " + mCurrentToken.mLexeme,
                            extra);
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
			throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
					"Expected a comma or right parenthesis in function call but received "
					+ mCurrentToken.mLexeme);
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
	throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
			"Expected 'ComparisonOperator' but received " + mCurrentToken.mLexeme,
			"ComparisonOperators are: ==, !=, >=, <=, <, or >");
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
		/// @todo @bug Consume the token here! @see ParseConstant()
		return new StringConstant(mCurrentToken.mLexeme);
	}
	throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
			"Expected a StringConstant.",
			"Received "+ mCurrentToken.mLexeme+" instead.");
}

Constant* TestDriver::ParseConstant()
{
	Constant* C = nullptr;

	if (mCurrentToken == TOK_INT)
		C = new Constant(new NumericConstant(atoi(mCurrentToken.mLexeme.c_str())));
	else
    if(mCurrentToken == TOK_FLOAT)
		C = new Constant(new NumericConstant((float)atof(mCurrentToken.mLexeme.c_str())));
	else
    if(mCurrentToken == TOK_STRING)
		C = new Constant(ParseStringConstant());
	else
    if(mCurrentToken == TOK_CHAR)
		// Watch for the index 1, the flex parser stores the char constant with
		// single quotes.
		// @bug What happens when we receive an escaped character?
		C = new Constant(new CharConstant(mCurrentToken.mLexeme[1]));
	else
		throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
				"Expected a Constant (int, float, string or char).",
				"Received "+ mCurrentToken.mLexeme+" instead.");
	// This is a workaround for float and double values. Let LLVM do the work
	// on what type of precision to choose, we will just pass a string representing
	// the floating point value.
	C->setAsStr(mCurrentToken.mLexeme);
	mCurrentToken = mTokenizer.nextToken(); // Consume the constant
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
	throw Exception("Expected { but received " + mCurrentToken.mLexeme + " after keyword 'after'");
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
            throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
                            "Expected a semi colon ';' at the end of the test expression",
                            "Received "+mCurrentToken.mLexeme);
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
	throw Exception("Expected { but received " + mCurrentToken.mLexeme + " after keyword 'before'");
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
			throw Exception(mCurrentToken.mLine,mCurrentToken.mColumn,
					"Expected a semi colon ';' at the end of the expression in test fixture",//@todo improve message
					"Received "+mCurrentToken.mLexeme);
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
		/// @bug Possible bug here, this else indicates in our grammar, a
		/// CONSTANT(INT), FLOAT, CHAR_CONST or STRING.
		Constant* C = ParseConstant();
		return new Operand(C);
	}
	throw Exception("Expected a CONSTANT or an IDENTIFIER");
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
		throw Exception("A mockup function needs an assignment operator, received " + mCurrentToken.mLexeme);
	}
	mCurrentToken = mTokenizer.nextToken(); // eat the '='

	Argument *argument = ParseArgument();

	return new MockupFunction(func, argument);
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
		} else if (mTokenizer.peekToken() == TOK_IDENTIFIER) {
			var = (MockupVariable*) ParseMockupVariable();
			MockupVariables.push_back(var);
		}

		if (mCurrentToken == '}')
			break;

		if (mCurrentToken != ';') {
                    for(auto*& ptr : MockupFunctions) delete ptr;
                    for(auto*& ptr : MockupVariables) delete ptr;
			throw Exception(mCurrentToken.mLine,mCurrentToken.mColumn,
					"Expected a semi colon ';' at the end of the test mockup",//@todo improve message
					"Received "+mCurrentToken.mLexeme);
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
	throw Exception("Excpected { but received " + mCurrentToken.mLexeme);
}

TestData* TestDriver::ParseTestData()
{
	if (mCurrentToken != TOK_TEST_DATA) {
		// @todo Create default information
		return nullptr;
	}
	mCurrentToken = mTokenizer.nextToken(); // eat the keyword data
	if (mCurrentToken != '{')
		throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
				"Expected { but received " + mCurrentToken.mLexeme);
	mCurrentToken = mTokenizer.nextToken(); // eat up the {

	unique_ptr<StringConstant> name = unique_ptr<StringConstant>(ParseStringConstant());
	// @todo This should happen inside ParseStringConstant
	mCurrentToken = mTokenizer.nextToken(); // eat up the StringConstant

	if (mCurrentToken != ';')
				throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
						"Expected ; but received " + mCurrentToken.mLexeme);
	mCurrentToken = mTokenizer.nextToken(); // eat up the }

	if (mCurrentToken != '}')
			throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
					"Expected } but received " + mCurrentToken.mLexeme);
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
		gm = ParseGlobalMockup();
		gs = ParseGlobalSetup();

		if(mCurrentToken == TOK_AFTER_ALL) {
			throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
					"The 'after_all' statement needs to be defined at the end"
					" of the group.");
		}
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
					throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
							"Invalid group definition. Expected a left curly bracket '{' for the given group",
							"Received: "+mCurrentToken.mLexeme+" instead.");
				}

				mCurrentToken = mTokenizer.nextToken();// eat up the '{'
				Identifier* ngn = new Identifier(name->getIdentifierStr()+":"+new_group_name->getIdentifierStr());
				delete new_group_name;
				TestGroup* group = ParseTestGroup(ngn);
				if (group)
					tests.push_back(group);

				if(mCurrentToken != '}') {
					delete ngn;
					delete group;
					throw Exception(mCurrentToken.mLine, mCurrentToken.mColumn,
									"Invalid group definition. Expected a right curly bracket '}'",
									"Received "+mCurrentToken.mLexeme+" instead.");
				}
				mCurrentToken = mTokenizer.nextToken(); // eat up the character '}'
			} else {
				TestDefinition *testDefinition = ParseTestDefinition();
				if(name)
					testDefinition->setGroupName(name->getIdentifierStr());
				else
					testDefinition->setGroupName("default");

				tests.push_back(testDefinition);
			}
		}

		////////////////////////////////////////
		// Changed the syntax and parse the groups teardown at the very end of the
		// group
		gt = ParseGlobalTeardown();
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

GlobalMockup* TestDriver::ParseGlobalMockup()
{
	if (mCurrentToken == TOK_MOCKUP_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			MockupFixture *mf = (MockupFixture*) ParseMockupFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalMockup(mf);
		}
		throw Exception("Expected { after mockup_all but received: " + mCurrentToken.mLexeme);
	}
	return nullptr;
}

GlobalSetup* TestDriver::ParseGlobalSetup()
{
	if (mCurrentToken == TOK_BEFORE_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixture *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalSetup(mf);
		}
		throw Exception("Expected { after before_all but received: " + mCurrentToken.mLexeme);
	}
	return nullptr;
}

GlobalTeardown* TestDriver::ParseGlobalTeardown()
{
	if (mCurrentToken == TOK_AFTER_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixture *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalTeardown(mf);
		}
		throw Exception("Expected { after after_all but received: " + mCurrentToken.mLexeme);
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
            throw Exception("Invalid test file");
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
		arg->setParent(this);
		arg->setIndex(i);
		++i;
	}
}

FunctionCall::~FunctionCall()
{
	delete mFunctionName;
	for (FunctionArgument*& ptr : mFunctionArguments) {
		delete ptr;
		ptr = nullptr;
	}
}

void FunctionCall::accept(Visitor *v)
{
	for (FunctionArgument*& ptr : mFunctionArguments) {
		ptr->accept(v);
	}
	v->VisitFunctionCall(this);
}

InitializerValue::~InitializerValue() {
	if (mArgValue) delete mArgValue;
	if (mStructValue) delete mStructValue;
}

void InitializerValue::accept(Visitor *v) {
	if (mArgValue) mArgValue->accept(v);
	if (mStructValue) mStructValue->accept(v);
	v->VisitInitializerValue(this);
}


string FunctionCall::getFunctionCalledString() {
	string called = mFunctionName->getIdentifierStr() +"(";
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

bool FunctionCall::hasDataPlaceholders() const {
	for(auto a : mFunctionArguments)
		if(a->isDataPlaceholder())
			return true;
	return false;
}

FunctionCall::FunctionCall(const FunctionCall& that)
: mFunctionName(nullptr), mFunctionArguments(), mReturnType(nullptr) {
	if (that.mFunctionName)
		mFunctionName = new Identifier(*that.mFunctionName);
	for(FunctionArgument* fa : that.mFunctionArguments)
		mFunctionArguments.push_back(new FunctionArgument(*fa));
}

InitializerValue::InitializerValue(const InitializerValue& that)
: mArgValue(nullptr), mStructValue(nullptr) {
	if (that.mArgValue)
		mArgValue = new Argument(*that.mArgValue);
	if (that.mStructValue)
		mStructValue = new StructInitializer(*that.mStructValue);
}
