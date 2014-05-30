#include "TestParser.h"
#include <iostream>
#include <exception>

using namespace std;
using namespace tp;

Tokenizer::Tokenizer(const string& filename) : mInput(filename),
mCurrentToken(TOK_ERR), mFunction(""), mEqOp('\0'), mInt(0), mDouble(0.0),
mBuffAlloc(""), mTokStrValue(""), mLastChar('\0')
{
	if (!mInput) {
		throw Exception("Could not open file " + filename);
	}

	mInput.seekg(0);
}

bool Tokenizer::isCharIgnored(char c)
{
	switch (c) {
	case ';':
		return true;
	default: return false;
	}
	return false;
}

int Tokenizer::peekToken()
{
	Token oldToken = mCurrentToken;
	char oldChar = mLastChar;
	string oldStr = mTokStrValue;
	int peek_token = nextToken();
	mInput.seekg(mOldPos);
	mCurrentToken = oldToken;
	mLastChar = oldChar;
	mTokStrValue = oldStr;
	return peek_token;
}

int Tokenizer::nextToken()
{
	stringstream tokenStream;
	mTokStrValue = "";
	mOldPos = mInput.tellg();
	mIdType = ID_UNKNOWN;
	mLastChar = mInput.get();
	while (isspace(mLastChar) || isCharIgnored(mLastChar))
		mLastChar = mInput.get();

	if (mLastChar == '"') {
		while ((mLastChar = mInput.get()) != '"') {
			mTokStrValue += mLastChar;
		}
		mIdType = ID_CONSTANT;
		return mCurrentToken = TOK_STRING;
	}

	if (mLastChar == '\'') {
		while ((mLastChar = mInput.get()) != '\'') {
			mTokStrValue += mLastChar;
		}
		return mCurrentToken = TOK_CHAR;
	}

	if (mLastChar == '{') {
		mTokStrValue = mLastChar;

		if (mCurrentToken == TOK_BEFORE || mCurrentToken == TOK_AFTER ||
				mCurrentToken == TOK_MOCKUP || mCurrentToken == TOK_MOCKUP_ALL ||
				mCurrentToken == TOK_AFTER_ALL || mCurrentToken == TOK_BEFORE_ALL ||
				mCurrentToken == TOK_GROUP || mCurrentToken == TOK_IDENTIFIER) {
			mCurrentToken = TOK_ASCII_CHAR;
			return mLastChar;
		}

		while ((mLastChar = mInput.get()) != '}') {
			mTokStrValue += mLastChar;
		}
		mTokStrValue += mLastChar;
		return mCurrentToken = TOK_ARRAY_INIT;
	}

	if (mLastChar == '}') {
		mTokStrValue = mLastChar;
		if (mCurrentToken == TOK_BEFORE || mCurrentToken == TOK_AFTER ||
				mCurrentToken == TOK_MOCKUP || mCurrentToken == TOK_MOCKUP_ALL ||
				mCurrentToken == TOK_AFTER_ALL || mCurrentToken == TOK_BEFORE_ALL) {
			mCurrentToken = TOK_ASCII_CHAR;
			mIdType = ID_CONSTANT;
			return mLastChar;
		}
	}

	if (mLastChar == '[') {
		mTokStrValue = mLastChar;
		while ((mLastChar = mInput.get()) != ']') {
			mTokStrValue += mLastChar;
		}
		mTokStrValue += mLastChar;
		return mCurrentToken = TOK_BUFF_ALLOC;
	}

	if (isalpha(mLastChar)) { // function: [a-zA-Z][a-zA-Z0-9]*
		mFunction = mLastChar;
		while (isalnum((mLastChar = mInput.get())) || mLastChar == '_') {
			mFunction += mLastChar;
		}
		if (mLastChar == '(') mIdType = ID_FUNCTION;
		else mIdType = ID_VARIABLE;
		mInput.putback(mLastChar);
		mTokStrValue = mFunction;
		if (mTokStrValue == "mockup") return mCurrentToken = TOK_MOCKUP;
		if (mTokStrValue == "before") return mCurrentToken = TOK_BEFORE;
		if (mTokStrValue == "after") return mCurrentToken = TOK_AFTER;
		if (mTokStrValue == "before_all") return mCurrentToken = TOK_BEFORE_ALL;
		if (mTokStrValue == "after_all") return mCurrentToken = TOK_AFTER_ALL;
		if (mTokStrValue == "mockup_all") return mCurrentToken = TOK_MOCKUP_ALL;
		if (mTokStrValue == "test") return mCurrentToken = TOK_TEST_INFO;
		if (mTokStrValue == "group") return mCurrentToken = TOK_GROUP;
		return mCurrentToken = TOK_IDENTIFIER;
	}

        /// @todo Add support for floating point numbers, detect the dot '.'
	if (((mLastChar == '-' && isdigit(mInput.peek())) || isdigit(mLastChar))) {
		tokenStream << mLastChar;
		while (isdigit((mLastChar = mInput.get()))) {
			tokenStream << mLastChar;
		}
		mInput.putback(mLastChar);
		tokenStream >> mInt;
		mTokStrValue = tokenStream.str();
		mIdType = ID_CONSTANT;
		return mCurrentToken = TOK_INT;
	}

	//////
	// Conditions for boolean operators
	if (mLastChar == '=' && mInput.peek() == '=') {
		tokenStream << mLastChar;
		tokenStream << (mLastChar = mInput.get());
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_COMPARISON_OP;
	}

	if (mLastChar == '!' && mInput.peek() == '=') {
		tokenStream << mLastChar;
		tokenStream << (mLastChar = mInput.get());
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_COMPARISON_OP;
	}

	if (mLastChar == '>' && mInput.peek() == '=') {
		tokenStream << mLastChar;
		tokenStream << (mLastChar = mInput.get());
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_COMPARISON_OP;
	}

	if (mLastChar == '<' && mInput.peek() == '=') {
		tokenStream << mLastChar;
		tokenStream << (mLastChar = mInput.get());
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_COMPARISON_OP;
	}

	if (mLastChar == '<') {
		tokenStream << mLastChar;
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_COMPARISON_OP;
	}

	if (mLastChar == '>') {
		tokenStream << mLastChar;
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_COMPARISON_OP;
	}
	// End of boolean operators
	//////
	if (mLastChar == '#') {
		mLastChar = mInput.get();
		while (mLastChar != '\n' && mLastChar != EOF && mLastChar != '\r') {
			mLastChar = mInput.get();
		}

		if (mLastChar != EOF)
			return nextToken();
	}

	if (mLastChar == EOF) {
		mTokStrValue = "EOF";
		return TOK_EOF;
	}

	mTokStrValue = mLastChar;
	mCurrentToken = TOK_ASCII_CHAR;
	return mLastChar;
}

//===-----------------------------------------------------------===//
//
// TestParser method definition
//

int TestExpr::leaks = 0;

Argument* TestDriver::ParseArgument()
{
	Argument *Number = new Argument(mTokenizer.getTokenStringValue(), (Tokenizer::Token)mCurrentToken);
	mCurrentToken = mTokenizer.nextToken(); // eat current argument, move to next
	return Number;
}

BufferAlloc* TestDriver::ParseBufferAlloc()
{
	BufferAlloc *Number = new BufferAlloc(mTokenizer.getTokenStringValue());
	mCurrentToken = mTokenizer.nextToken(); // eat current argument, move to next
	return Number;
}

Identifier* TestDriver::ParseIdentifier()
{
	if (mCurrentToken != Tokenizer::TOK_IDENTIFIER)
		throw Exception("Expected an identifier but received " + mTokenizer.getTokenStringValue());
	Identifier * id = new Identifier(mTokenizer.getTokenStringValue());
	mCurrentToken = mTokenizer.nextToken(); // eat current identifier
	return id;
}

FunctionArgument* TestDriver::ParseFunctionArgument()
{
	if (mCurrentToken == Tokenizer::TOK_BUFF_ALLOC)
		return new FunctionArgument(ParseBufferAlloc());
	else
		return new FunctionArgument(ParseArgument());
	throw Exception("Expected a valid Argument but received: " + mTokenizer.getTokenStringValue());
}

VariableAssignment* TestDriver::ParseVariableAssignment()
{
	Identifier *identifier = ParseIdentifier();

	if (mCurrentToken != '=') {
		delete identifier;
		throw Exception("Expected = but received " + mTokenizer.getTokenStringValue());
	}
	mCurrentToken = mTokenizer.nextToken(); // eat up the '='

	Argument *arg = ParseArgument();

	return new VariableAssignment(identifier, arg);
}

Identifier* TestDriver::ParseFunctionName()
{
	if (mCurrentToken != Tokenizer::TOK_IDENTIFIER)
		throw Exception("Expected an identifier name but received " + mTokenizer.getTokenStringValue());
	return ParseIdentifier(); // Missing a 'FunctionName' class that wraps an Identifier
}

FunctionCall* TestDriver::ParseFunctionCall()
{
	Identifier* functionName = ParseFunctionName();

	if (mCurrentToken != '(')
		throw Exception("Expected ( but received " + mTokenizer.getTokenStringValue());

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
			throw Exception("Expected , or ) but received " + mTokenizer.getTokenStringValue());
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
	if (mCurrentToken == Tokenizer::TOK_COMPARISON_OP) {
		ComparisonOperator* cmp = new ComparisonOperator(mTokenizer.getTokenStringValue());
		mCurrentToken = mTokenizer.nextToken(); // consume TOK_COMPARISON_OP
		return cmp;
	}
	throw Exception("Expected 'ComparisonOperator' but received token " + mTokenizer.getTokenStringValue());
}

ExpectedConstant* TestDriver::ParseExpectedConstant()
{
    return new ExpectedConstant(ParseConstant());
}

Constant* TestDriver::ParseConstant()
{
	Constant* C = nullptr;

	if (mCurrentToken == Tokenizer::TOK_INT)
		C = new Constant(new NumericConstant(mTokenizer.getInteger()));
	else
    if(mCurrentToken == Tokenizer::TOK_DOUBLE)// @TODO Refactor name double to float
		C = new Constant(new NumericConstant(mTokenizer.getDouble()));
	else
    if(mCurrentToken == Tokenizer::TOK_STRING)
		C = new Constant(new StringConstant(mTokenizer.getTokenStringValue()));
	else
    if(mCurrentToken == Tokenizer::TOK_CHAR)
		C = new Constant(new CharConstant(mTokenizer.getChar()));
	else
		throw Exception("Expected a Constant but received token " + mTokenizer.getTokenStringValue());

	mCurrentToken = mTokenizer.nextToken(); // Consume the constant
	return C;
}

TestTeardown* TestDriver::ParseTestTearDown()
{
	if (mCurrentToken != Tokenizer::TOK_AFTER)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'after'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		TestFixture* testFixture = ParseTestFixture();
		mCurrentToken = mTokenizer.nextToken(); //eat the  '}'
		return new TestTeardown(testFixture);
	}
	throw Exception("Expected { but received " + mTokenizer.getTokenStringValue() + " after keyword 'after'");
}

TestFunction* TestDriver::ParseTestFunction()
{
	FunctionCall *FunctionCall = ParseFunctionCall();
	ExpectedResult* ER = nullptr;
	if (mCurrentToken == Tokenizer::TOK_COMPARISON_OP)
		ER = ParseExpectedResult();
	return new TestFunction(FunctionCall, ER);
}

TestSetup* TestDriver::ParseTestSetup()
{
	if (mCurrentToken != Tokenizer::TOK_BEFORE)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'before'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		TestFixture* testFixture = ParseTestFixture();
		mCurrentToken = mTokenizer.nextToken(); //eat the  '}'
		return new TestSetup(testFixture);
	}
	throw Exception("Expected { but received " + mTokenizer.getTokenStringValue() + " after keyword 'before'");
}

TestFixture* TestDriver::ParseTestFixture()
{
	vector<VariableAssignment*> assignments;
	vector<FunctionCall*> functions;
	vector<ExpectedExpression*> expected;
	FunctionCall *func = nullptr;
	VariableAssignment* var = nullptr;
	ExpectedExpression* exp = nullptr;

	while (mCurrentToken != '}') {
		if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_FUNCTION) {
			func = (FunctionCall*) ParseFunctionCall();
			functions.push_back(func);
			func = nullptr;
		} else if (mTokenizer.getIdentifierType() == Tokenizer::ID_VARIABLE) {
			if (mTokenizer.peekToken() == Tokenizer::TOK_COMPARISON_OP) {
				exp = ParseExpectedExpression();
				expected.push_back(exp);
				exp = nullptr;
			} else if(mTokenizer.peekToken() == '=') {
				var = (VariableAssignment*) ParseVariableAssignment();
				assignments.push_back(var);
				var = nullptr;
			}
		} else if (mTokenizer.getIdentifierType() == Tokenizer::ID_CONSTANT) {
			exp = ParseExpectedExpression();
			expected.push_back(exp);
			exp = nullptr;
		}

		if (mCurrentToken == '}')
			break;
	}
	return new TestFixture(functions, assignments, expected);
}

ExpectedExpression* TestDriver::ParseExpectedExpression()
{
	Operand* LHS = ParseOperand();
	ComparisonOperator* CO = ParseComparisonOperator();
	Operand* RHS = ParseOperand();
	return new ExpectedExpression(LHS, CO, RHS);
}

Operand* TestDriver::ParseOperand()
{
	if(mTokenizer.getIdentifierType() == Tokenizer::ID_CONSTANT) {
		Constant* C = ParseConstant();
		return new Operand(C);
	} else if(mCurrentToken == Tokenizer::TOK_IDENTIFIER) {
		Identifier* I = ParseIdentifier();
		return new Operand(I);
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
		throw Exception("A mockup function needs an assignment operator, received " + mTokenizer.getTokenStringValue());
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
		if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_FUNCTION) {
			func = (MockupFunction*) ParseMockupFunction();
			MockupFunctions.push_back(func);
		} else if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_VARIABLE) {
			var = (MockupVariable*) ParseMockupVariable();
			MockupVariables.push_back(var);
		}

		if (mCurrentToken == '}')
			break;
	}
	return new MockupFixture(MockupFunctions, MockupVariables);
}

TestMockup* TestDriver::ParseTestMockup()
{
	if (mCurrentToken != Tokenizer::TOK_MOCKUP)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'mockup'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		MockupFixture* mf = (MockupFixture*) ParseMockupFixture();
		mCurrentToken = mTokenizer.nextToken(); // eat the '}'
		return new TestMockup(mf);
	}
	throw Exception("Excpected { but received " + mTokenizer.getTokenStringValue());
}

TestInfo* TestDriver::ParseTestInfo()
{
	if (mCurrentToken != Tokenizer::TOK_TEST_INFO) {
		// @todo Create default information
		return nullptr;
	}
	mCurrentToken = mTokenizer.nextToken(); // eat the keyword test
	Identifier* name = ParseIdentifier();
	return new TestInfo(name);
}

TestDefinition* TestDriver::ParseTestDefinition()
{
	TestInfo *info = ParseTestInfo();
	TestMockup *mockup = ParseTestMockup();
	TestSetup *setup = ParseTestSetup();
	TestFunction *testFunction = ParseTestFunction();
	TestTeardown *teardown = ParseTestTearDown();

	return new TestDefinition(info, testFunction, setup, teardown, mockup);
}

TestGroup* TestDriver::ParseTestGroup()
{
	vector<TestDefinition*> definitions;
	vector<TestGroup*> groups;
	Identifier* name = nullptr;

	while (true) {
		try {
			if (mCurrentToken == '}' or mCurrentToken == Tokenizer::TOK_EOF)
				break;

			if (mCurrentToken == Tokenizer::TOK_GROUP) {

				mCurrentToken = mTokenizer.nextToken(); // eat up the group keyword

				if (mCurrentToken == Tokenizer::TOK_IDENTIFIER)
					name = ParseIdentifier();

				if (mCurrentToken != '{')
					throw Exception("Expected a '{'  for the given group, but received: "+mTokenizer.getTokenStringValue());

				mCurrentToken = mTokenizer.nextToken();// eat up the '{'
				TestGroup* group = ParseTestGroup();
				groups.push_back(group);
			} else {
				TestDefinition *TestDefinition = ParseTestDefinition();
				definitions.push_back(TestDefinition);
			}
		} catch (const exception& e) {
			cerr << e.what() << endl;
			// Let's try to recover until the next test inside this group
			while (mCurrentToken != Tokenizer::TOK_IDENTIFIER and
					mCurrentToken != Tokenizer::TOK_EOF)
				mCurrentToken = mTokenizer.nextToken();
		}
	}

	mCurrentToken = mTokenizer.nextToken(); // eat up the character '}'
	return new TestGroup(name, definitions, groups);
}

UnitTests* TestDriver::ParseUnitTest()
{
	vector<TestGroup*> groups;

	if (mCurrentToken != Tokenizer::TOK_IDENTIFIER &&
			mCurrentToken != Tokenizer::TOK_BEFORE &&
			mCurrentToken != Tokenizer::TOK_MOCKUP &&
			mCurrentToken != Tokenizer::TOK_TEST_INFO &&
			mCurrentToken != Tokenizer::TOK_GROUP)
		throw Exception("Expected a function call");

	if (mCurrentToken == Tokenizer::TOK_IDENTIFIER or
			mCurrentToken == Tokenizer::TOK_BEFORE or
			mCurrentToken == Tokenizer::TOK_MOCKUP or
			mCurrentToken == Tokenizer::TOK_TEST_INFO or
			mCurrentToken == Tokenizer::TOK_GROUP) {
		TestGroup* group = ParseTestGroup();
		groups.push_back(group);
	}
	return new UnitTests(groups);
}

GlobalMockup* TestDriver::ParseGlobalMockup()
{
	if (mCurrentToken == Tokenizer::TOK_MOCKUP_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			MockupFixture *mf = (MockupFixture*) ParseMockupFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalMockup(mf);
		}
		throw Exception("Expected { after mockup_all but received: " + mTokenizer.getTokenStringValue());
	}
	return nullptr;
}

GlobalSetup* TestDriver::ParseGlobalSetup()
{
	if (mCurrentToken == Tokenizer::TOK_BEFORE_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixture *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalSetup(mf);
		}
		throw Exception("Expected { after before_all but received: " + mTokenizer.getTokenStringValue());
	}
	return nullptr;
}

GlobalTeardown* TestDriver::ParseGlobalTeardown()
{
	if (mCurrentToken == Tokenizer::TOK_AFTER_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixture *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalTeardown(mf);
		}
		throw Exception("Expected { after after_all but received: " + mTokenizer.getTokenStringValue());
	}
	return nullptr;
}

TestFile* TestDriver::ParseTestFile()
{
	GlobalMockup *gm = ParseGlobalMockup();
	GlobalSetup *gs = ParseGlobalSetup();
	GlobalTeardown *gt = ParseGlobalTeardown();
	UnitTests *test = ParseUnitTest();
	return new TestFile(test, gm, gs, gt);
}

TestExpr* TestDriver::ParseTestExpr()
{
	// Position on the first token
	mCurrentToken = mTokenizer.nextToken();
	return ParseTestFile();
}

// Definitions due to conflicts with the forward declarations
FunctionCall::FunctionCall(Identifier* name, const vector<FunctionArgument*>& arg) :
FunctionName(name), FunctionArguments(arg)
{
	unsigned i = 0;
	for (auto*& ptr : FunctionArguments) {
		if (FunctionArgument * arg = dynamic_cast<FunctionArgument*> (ptr)) {
			arg->setParent(this);
			arg->setIndex(i);
		}
		++i;
	}
}

FunctionCall::~FunctionCall()
{
	delete FunctionName;
	for (FunctionArgument*& ptr : FunctionArguments) {
		delete ptr;
		ptr = nullptr;
	}
}

void FunctionCall::dump()
{
	FunctionName->dump();
	cout << "(";
	for (auto arg : FunctionArguments) {
		arg->dump();
		cout << ",";
	}
	cout << ")";
}

void FunctionCall::accept(Visitor *v)
{
	for (FunctionArgument*& ptr : FunctionArguments) {
		ptr->accept(v);
	}
	v->VisitFunctionCall(this);
}