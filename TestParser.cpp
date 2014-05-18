#include "TestParser.hxx"
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

int Tokenizer::nextToken()
{
	stringstream tokenStream;
	mTokStrValue = "";
	mLastChar = mInput.get();
	while (isspace(mLastChar) || isCharIgnored(mLastChar))
		mLastChar = mInput.get();

	if (mLastChar == '"') {
		while ((mLastChar = mInput.get()) != '"') {
			mTokStrValue += mLastChar;
		}
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
				mCurrentToken == TOK_AFTER_ALL || mCurrentToken == TOK_BEFORE_ALL) {
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
		return mCurrentToken = TOK_IDENTIFIER;
	}

	if (((mLastChar == '-' && isdigit(mInput.peek())) || isdigit(mLastChar))) {
		tokenStream << mLastChar;
		while (isdigit((mLastChar = mInput.get()))) {
			tokenStream << mLastChar;
		}
		mInput.putback(mLastChar);
		tokenStream >> mInt;
		mTokStrValue = tokenStream.str();
		return mCurrentToken = TOK_INT;
	}

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

VariableAssignmentExpr* TestDriver::ParseVariableAssignment()
{
	Identifier *identifier = ParseIdentifier();

	if (mCurrentToken != '=') {
		delete identifier;
		throw Exception("Expected = but received " + mTokenizer.getTokenStringValue());
	}
	mCurrentToken = mTokenizer.nextToken(); // eat up the '='

	Argument *arg = ParseArgument();

	return new VariableAssignmentExpr(identifier, arg);
}

Identifier* TestDriver::ParseFunctionName()
{
	if (mCurrentToken != Tokenizer::TOK_IDENTIFIER)
		throw Exception("Expected an identifier name but received " + mTokenizer.getTokenStringValue());
	return ParseIdentifier(); // Missing a 'FunctionName' class that wraps an Identifier
}

FunctionCallExpr* TestDriver::ParseFunctionCall()
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

	return new FunctionCallExpr(functionName, Args);
}

TestTeardownExpr* TestDriver::ParseTestTearDown()
{
	if (mCurrentToken != Tokenizer::TOK_AFTER)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'after'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		TestFixtureExpr* testFixture = ParseTestFixture();
		mCurrentToken = mTokenizer.nextToken(); //eat the  '}'
		return new TestTeardownExpr(testFixture);
	}
	throw Exception("Expected { but received " + mTokenizer.getTokenStringValue() + " after keyword 'after'");
}

TestFunction* TestDriver::ParseTestFunction()
{
	FunctionCallExpr *FunctionCall = ParseFunctionCall();
	return new TestFunction(FunctionCall);
}

TestSetupExpr* TestDriver::ParseTestSetup()
{
	if (mCurrentToken != Tokenizer::TOK_BEFORE)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'before'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		TestFixtureExpr* testFixture = ParseTestFixture();
		mCurrentToken = mTokenizer.nextToken(); //eat the  '}'
		return new TestSetupExpr(testFixture);
	}
	throw Exception("Expected { but received " + mTokenizer.getTokenStringValue() + " after keyword 'before'");
}

TestFixtureExpr* TestDriver::ParseTestFixture()
{
	vector<VariableAssignmentExpr*> assignments;
	vector<FunctionCallExpr*> functions;
	FunctionCallExpr *func = nullptr;
	VariableAssignmentExpr* var = nullptr;
	while (mCurrentToken != '}') {
		if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_FUNCTION) {
			func = (FunctionCallExpr*) ParseFunctionCall();
			functions.push_back(func);
			func = nullptr;
		} else if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_VARIABLE) {
			var = (VariableAssignmentExpr*) ParseVariableAssignment();
			assignments.push_back(var);
			var = nullptr;
		}

		if (mCurrentToken == '}')
			break;
	}
	return new TestFixtureExpr(functions, assignments);
}

MockupVariableExpr* TestDriver::ParseMockupVariable()
{
	VariableAssignmentExpr *varAssign = ParseVariableAssignment();
	return new MockupVariableExpr(varAssign);
}

MockupFunctionExpr* TestDriver::ParseMockupFunction()
{
	FunctionCallExpr *func = ParseFunctionCall();

	if (mCurrentToken != '=') {
		delete func;
		throw Exception("A mockup function needs an assignment operator, received " + mTokenizer.getTokenStringValue());
	}
	mCurrentToken = mTokenizer.nextToken(); // eat the '='

	Argument *argument = ParseArgument();

	return new MockupFunctionExpr(func, argument);
}

MockupFixtureExpr* TestDriver::ParseMockupFixture()
{
	vector<MockupFunctionExpr*> MockupFunctions;
	vector<MockupVariableExpr*> MockupVariables;

	while (mCurrentToken != '}') {

		MockupFunctionExpr *func = nullptr;
		MockupVariableExpr *var = nullptr;
		if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_FUNCTION) {
			func = (MockupFunctionExpr*) ParseMockupFunction();
			MockupFunctions.push_back(func);
		} else if (mTokenizer.getIdentifierType() == Tokenizer::Identifier::ID_VARIABLE) {
			var = (MockupVariableExpr*) ParseMockupVariable();
			MockupVariables.push_back(var);
		}

		if (mCurrentToken == '}')
			break;
	}
	return new MockupFixtureExpr(MockupFunctions, MockupVariables);
}

TestMockupExpr* TestDriver::ParseTestMockup()
{
	if (mCurrentToken != Tokenizer::TOK_MOCKUP)
		return nullptr;

	mCurrentToken = mTokenizer.nextToken(); // eat the keyword 'mockup'

	if (mCurrentToken == '{') {
		mCurrentToken = mTokenizer.nextToken(); // eat the '{'
		MockupFixtureExpr* mf = (MockupFixtureExpr*) ParseMockupFixture();
		mCurrentToken = mTokenizer.nextToken(); // eat the '}'
		return new TestMockupExpr(mf);
	}
	throw Exception("Excpected { but received " + mTokenizer.getTokenStringValue());
}

TestDefinitionExpr* TestDriver::ParseTestDefinition()
{
	TestMockupExpr *mockup = ParseTestMockup();
	TestSetupExpr *setup = ParseTestSetup();
	TestFunction *testFunction = ParseTestFunction();
	TestTeardownExpr *teardown = nullptr;

	// Means the user skipped the expected result part but provided an after keyword
	if (mCurrentToken == Tokenizer::TOK_AFTER)
		teardown = ParseTestTearDown();

	// The assignment part is optional, entering this if means the user skipped it
	if (mCurrentToken == Tokenizer::TOK_IDENTIFIER or
			mCurrentToken == Tokenizer::TOK_BEFORE or
			mCurrentToken == Tokenizer::TOK_MOCKUP or
			mCurrentToken == Tokenizer::TOK_EOF)
		return new TestDefinitionExpr(testFunction, nullptr, setup, teardown, mockup);

	if (mCurrentToken != '=') { // we expect an assignment at this point
		if (testFunction) delete testFunction;
		if (setup) delete setup;
		if (mockup) delete mockup;
		throw Exception("Expected = but received " + mTokenizer.getTokenStringValue());
	}
	mCurrentToken = mTokenizer.nextToken(); // consume '='

	// The only allowed after an '=' is an Argument
	if (mCurrentToken == Tokenizer::TOK_BUFF_ALLOC) {
		if (testFunction) delete testFunction;
		if (setup) delete setup;
		if (mockup) delete mockup;
		throw Exception("Expected an Argument but received a BufferAlloc");
	}
	Argument *ExpectedResult = ParseArgument();

	// User provided an expected result and also an after part
	if (mCurrentToken == Tokenizer::TOK_AFTER)
		teardown = ParseTestTearDown();

	return new TestDefinitionExpr(testFunction, ExpectedResult, setup, teardown, mockup);
}

UnitTestExpr* TestDriver::ParseUnitTestExpr()
{
	vector<TestDefinitionExpr*> definitions;

	if (mCurrentToken != Tokenizer::TOK_IDENTIFIER &&
			mCurrentToken != Tokenizer::TOK_BEFORE &&
			mCurrentToken != Tokenizer::TOK_MOCKUP)
		throw Exception("Expected a function call");

	if (mCurrentToken == Tokenizer::TOK_IDENTIFIER or
			mCurrentToken == Tokenizer::TOK_BEFORE or
			mCurrentToken == Tokenizer::TOK_MOCKUP) {
		while (true) {
			try {
				TestDefinitionExpr *TestDefinition = ParseTestDefinition();
				definitions.push_back(TestDefinition);
				if (mCurrentToken == Tokenizer::TOK_EOF)
					break;
			} catch (const exception& e) {
				cerr << e.what() << endl;
				// Let's try to recover until the next test
				while (mCurrentToken != Tokenizer::TOK_IDENTIFIER)
					mCurrentToken = mTokenizer.nextToken();
			}
		}
	}
	return new UnitTestExpr(definitions);
}

GlobalMockupExpr* TestDriver::ParseGlobalMockupExpr()
{
	if (mCurrentToken == Tokenizer::TOK_MOCKUP_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			MockupFixtureExpr *mf = (MockupFixtureExpr*) ParseMockupFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalMockupExpr(mf);
		}
		throw Exception("Expected { after mockup_all but received: " + mTokenizer.getTokenStringValue());
	}
	return nullptr;
}

GlobalSetupExpr* TestDriver::ParseGlobalSetupExpr()
{
	if (mCurrentToken == Tokenizer::TOK_BEFORE_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixtureExpr *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalSetupExpr(mf);
		}
		throw Exception("Expected { after before_all but received: " + mTokenizer.getTokenStringValue());
	}
	return nullptr;
}

GlobalTeardownExpr* TestDriver::ParseGlobalTeardownExpr()
{
	if (mCurrentToken == Tokenizer::TOK_AFTER_ALL) {
		mCurrentToken = mTokenizer.nextToken(); // eat keyword 'mockup_all'
		if (mCurrentToken == '{') {
			mCurrentToken = mTokenizer.nextToken(); // eat the '{'
			TestFixtureExpr *mf = ParseTestFixture();
			mCurrentToken = mTokenizer.nextToken(); // eat the '}'
			return new GlobalTeardownExpr(mf);
		}
		throw Exception("Expected { after after_all but received: " + mTokenizer.getTokenStringValue());
	}
	return nullptr;
}

TestFile* TestDriver::ParseTestFile()
{
	GlobalMockupExpr *gm = ParseGlobalMockupExpr();
	GlobalSetupExpr *gs = ParseGlobalSetupExpr();
	GlobalTeardownExpr *gt = ParseGlobalTeardownExpr();
	UnitTestExpr *test = ParseUnitTestExpr();
	return new TestFile(test, gm, gs, gt);
}

TestExpr* TestDriver::ParseTestExpr()
{
	// Position on the first token
	mCurrentToken = mTokenizer.nextToken();
	return ParseTestFile();
}

// Definitions due to conflicts with the forward declarations
FunctionCallExpr::FunctionCallExpr(Identifier* name, const vector<FunctionArgument*>& arg) :
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

FunctionCallExpr::~FunctionCallExpr()
{
	delete FunctionName;
	for (FunctionArgument*& ptr : FunctionArguments) {
		delete ptr;
		ptr = nullptr;
	}
}

void FunctionCallExpr::dump()
{
	FunctionName->dump();
	cout << "(";
	for (auto arg : FunctionArguments) {
		arg->dump();
		cout << ",";
	}
	cout << ")";
}

void FunctionCallExpr::accept(Visitor *v)
{
	for (FunctionArgument*& ptr : FunctionArguments) {
		ptr->accept(v);
	}
	v->VisitFunctionCallExpr(this);
}