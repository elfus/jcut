#include "TestParser.h"
#include <iostream>
#include <exception>

using namespace std;
using namespace tp;

int TestGroup::group_count = 0;
string Exception::mCurrentFile;

Tokenizer::Tokenizer(const string& filename) : mInput(filename),
mCurrentToken(TOK_ERR), mFunction(""), mEqOp('\0'), mInt(0), mFloat(0.0),
mBuffAlloc(""), mTokStrValue(""), mLastChar('\0'), mLineNum(1), mColumnNum(1),
mPrevLine(0), mPrevCol(0)
{
	if (!mInput) {
		throw Exception("Could not open file " + filename);
	}

	mInput.seekg(0);
}

bool Tokenizer::isCharIgnored(char c)
{
	switch (c) {
	default: return false;
	}
	return false;
}

int Tokenizer::peekToken()
{
	Token oldToken = mCurrentToken;
	char oldChar = mLastChar;
	string oldStr = mTokStrValue;
	unsigned oldPrevLine = mPrevLine;
	unsigned oldPrevCol = mPrevCol;
	unsigned oldLine = mLineNum;
	unsigned oldCol = mColumnNum;
	int peek_token = nextToken();
	mColumnNum = oldCol;
	mLineNum = oldLine;
	mPrevCol = oldPrevCol;
	mPrevLine = oldPrevLine;
	mInput.seekg(mOldPos);
	mCurrentToken = oldToken;
	mLastChar = oldChar;
	mTokStrValue = oldStr;
	return peek_token;
}

int Tokenizer::nextToken()
{
	stringstream tokenStream;
	static unsigned last_nl = 0;// Position of the last \n detected.
	mTokStrValue = "";
	mOldPos = mInput.tellg();
	mIdType = ID_UNKNOWN;

	do {
		mColumnNum = static_cast<unsigned>(mInput.tellg()) - last_nl + 1;
		mLastChar = mInput.get();
		if (mLastChar == '\n' ) {
			mPrevLine = mLineNum++;
			mPrevCol = mColumnNum;
			mColumnNum = 1;
			last_nl = mInput.tellg();
		}
		if (mLastChar == EOF)
			mColumnNum = 1;
	} while (isspace(mLastChar) || isCharIgnored(mLastChar));

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

		// Watch for possible @bug here. This assumes the character before '{'
		// was an assignment operator '='
		if (mCurrentToken == TOK_ASCII_CHAR) { // mCurrentToken == '='
			mTokStrValue = mLastChar;
			return mCurrentToken = TOK_STRUCT_INIT;
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

	// @todo remove this condition, this should be returned as a char constant
	// at the very end of this function. We have to replace the usage of
	// TOK_BUFF_ALLOC
	if (mLastChar == '[') {
		mTokStrValue = mLastChar;
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
		if (mLastChar != '.') {
			mInput.putback(mLastChar);
			tokenStream >> mInt;
			mTokStrValue = tokenStream.str();
			mIdType = ID_CONSTANT;
			return mCurrentToken = TOK_INT;
		} else { // when equals to '.' it's floating type value
			tokenStream << mLastChar;
			while (isdigit((mLastChar = mInput.get()))) {
				tokenStream << mLastChar;
			}
			mInput.putback(mLastChar);
			tokenStream >> mFloat;
			mTokStrValue = tokenStream.str();
			mIdType = ID_CONSTANT;
			return mCurrentToken = TOK_FLOAT;
		}
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
		mPrevLine = mLineNum++;
		mPrevCol = mColumnNum;
		mColumnNum = 1;
		last_nl = mInput.tellg();

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
	if (mCurrentToken != Tokenizer::TOK_BUFF_ALLOC) // @todo change to '['
		throw Exception("BufferAlloc",mTokenizer.getTokenStringValue());
	mCurrentToken = mTokenizer.nextToken(); // eat up the '['

	if (mCurrentToken != Tokenizer::TOK_INT)
		throw Exception("buffer size",mTokenizer.getTokenStringValue());
	// @todo Create an integer class and stop using argument
	Argument* buff_size = ParseArgument();// Buffer Size

	BufferAlloc *ba = nullptr;
	if (mCurrentToken == ':') {
		mCurrentToken = mTokenizer.nextToken(); // eat up the ':'
		if (mCurrentToken == Tokenizer::TOK_STRUCT_INIT) {
			StructInitializer* struct_init = ParseStructInitializer();
			ba = new BufferAlloc(buff_size, struct_init);
		} else {
			Argument* default_val = ParseArgument();
			ba = new BufferAlloc(buff_size, default_val);
		}
		mCurrentToken = mTokenizer.nextToken(); // eat up the ']'
		return ba;
	}
	mCurrentToken = mTokenizer.nextToken(); // eat up the ']'
	ba = new BufferAlloc(buff_size);// default value to 0
	return ba;
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

InitializerValue* TestDriver::ParseInitializerValue()
{
	if (mCurrentToken == Tokenizer::TOK_STRUCT_INIT) {
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
			throw Exception("Unexpected token: "+mTokenizer.getTokenStringValue());
		}
		
		mCurrentToken = mTokenizer.nextToken(); // eat up the '.'
		id = ParseIdentifier();
		if (mCurrentToken != '=') {
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
	if (mCurrentToken != Tokenizer::TOK_STRUCT_INIT)
		throw Exception("Expected struct initializer '{' but received "+mTokenizer.getTokenStringValue());
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
		delete si;
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
		throw Exception("Expected = but received " + mTokenizer.getTokenStringValue());
	}
	mCurrentToken = mTokenizer.nextToken(); // eat up the '='

	if (mCurrentToken == Tokenizer::TOK_STRUCT_INIT) {
		StructInitializer* struct_init = ParseStructInitializer();
		return new VariableAssignment(identifier, struct_init);
	}

	if (mCurrentToken == Tokenizer::TOK_BUFF_ALLOC) {
		BufferAlloc* ba = ParseBufferAlloc();
		return new VariableAssignment(identifier, ba);
	}

	Argument *arg = ParseArgument();
	return new VariableAssignment(identifier, arg);
}

Identifier* TestDriver::ParseFunctionName()
{
	if (mCurrentToken != Tokenizer::TOK_IDENTIFIER)
		throw Exception(mTokenizer.line(),mTokenizer.column(),
				"Expected a valid function name.",
				"Received "+ mTokenizer.getTokenStringValue()+" instead.");
	return ParseIdentifier(); // Missing a 'FunctionName' class that wraps an Identifier
}

FunctionCall* TestDriver::ParseFunctionCall()
{
	Identifier* functionName = ParseFunctionName();

	if (mCurrentToken != '(')
		throw Exception(mTokenizer.line(),mTokenizer.column(),
				"Expected a left parenthesis in function call but received " + mTokenizer.getTokenStringValue(),
				functionName->getIdentifierStr()+mTokenizer.getTokenStringValue());

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
			throw Exception(mTokenizer.line(), mTokenizer.column(),
					"Expected a comma or right parenthesis in function call but received "
					+ mTokenizer.getTokenStringValue());
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
	throw Exception(mTokenizer.line(), mTokenizer.column(),
			"Expected 'ComparisonOperator' but received " + mTokenizer.getTokenStringValue(),
			"ComparisonOperators are: ==, !=, >=, <=, <, or >");
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
    if(mCurrentToken == Tokenizer::TOK_FLOAT)
		C = new Constant(new NumericConstant(mTokenizer.getFloat()));
	else
    if(mCurrentToken == Tokenizer::TOK_STRING)
		C = new Constant(new StringConstant(mTokenizer.getTokenStringValue()));
	else
    if(mCurrentToken == Tokenizer::TOK_CHAR)
		C = new Constant(new CharConstant(mTokenizer.getTokenStringValue()[0]));
	else
		throw Exception(mTokenizer.line(), mTokenizer.column(),
				"Expected a Constant (int, float, string or char).",
				"Received "+ mTokenizer.getTokenStringValue()+" instead.");
	// This is a workaround for float and double values. Let LLVM do the work
	// on what type of precision to choose, we will just pass a string representing
	// the floating point value.
	C->setAsStr(mTokenizer.getTokenStringValue());
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
	if (mCurrentToken != ';') 
		throw Exception(mTokenizer.previousLine(), mTokenizer.previousColumn(),
				"Expected a semi colon ';' at the end of the test expression",
				"Received "+mTokenizer.getTokenStringValue());
	mCurrentToken = mTokenizer.nextToken(); //eat up the ';'
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

		if (mCurrentToken != ';')
			throw Exception(mTokenizer.previousLine(),mTokenizer.previousColumn(),
					"Expected a semi colon ';' at the end of the expression in test fixture",//@todo improve message
					"Received "+mTokenizer.getTokenStringValue());

		if (mCurrentToken == ';')
			mCurrentToken = mTokenizer.nextToken(); // eat up the ';'
		
		if (mCurrentToken == '}') {
			mCurrentToken = mTokenizer.nextToken(); // eat up the '}'
			break;
		}
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

		if (mCurrentToken != ';')
			throw Exception(mTokenizer.line(),mTokenizer.column(),
					"Expected a semi colon ';' at the end of the test mockup",//@todo improve message
					"Received "+mTokenizer.getTokenStringValue());
		mCurrentToken = mTokenizer.nextToken(); //eat up the ';'
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

TestGroup* TestDriver::ParseTestGroup(Identifier* name)
{
	GlobalMockup *gm = ParseGlobalMockup();
	GlobalSetup *gs = ParseGlobalSetup();
	GlobalTeardown *gt = ParseGlobalTeardown();
	vector<TestExpr*> tests;
	Identifier* new_group_name = nullptr;
	bool group_exception = false;

	while (true) {
		try {
			if (mCurrentToken == '}' or mCurrentToken == Tokenizer::TOK_EOF)
				break;

			if (mCurrentToken == Tokenizer::TOK_GROUP) {

				mCurrentToken = mTokenizer.nextToken(); // eat up the group keyword

				if (mCurrentToken == Tokenizer::TOK_IDENTIFIER)
					new_group_name = ParseIdentifier();

				if (mCurrentToken != '{') {
					group_exception = true;
					throw Exception(mTokenizer.previousLine(), mTokenizer.previousLine(),
							"Invalid group definition. Expected a left curly bracket '{' for the given group",
							"Received: "+mTokenizer.getTokenStringValue()+" instead.");
				}

				mCurrentToken = mTokenizer.nextToken();// eat up the '{'
				TestGroup* group = ParseTestGroup(new_group_name);
				tests.push_back(group);

				if(mCurrentToken != '}')
					throw Exception(mTokenizer.previousLine(), mTokenizer.previousColumn(),
							"Invalid group definition. Expected a right curly bracket '}'",
							"Received "+mTokenizer.getTokenStringValue()+" instead.");
				mCurrentToken = mTokenizer.nextToken(); // eat up the character '}'
			} else {
				TestDefinition *TestDefinition = ParseTestDefinition();
				tests.push_back(TestDefinition);
			}
		} catch (const exception& e) {
			cerr << e.what() << endl;
			// Let's try to recover until the next test inside this group
			while(group_exception == false and
					mCurrentToken != Tokenizer::TOK_TEST_INFO and
					mCurrentToken != Tokenizer::TOK_MOCKUP and
					mCurrentToken != Tokenizer::TOK_BEFORE and
					mCurrentToken != Tokenizer::TOK_IDENTIFIER and
					mCurrentToken != Tokenizer::TOK_GROUP and
					mCurrentToken != Tokenizer::TOK_EOF) {
				mCurrentToken = mTokenizer.nextToken();
			}
			
			if(group_exception) {
				while(mCurrentToken != '}')
					mCurrentToken = mTokenizer.nextToken();
				mCurrentToken = mTokenizer.nextToken();// eat up the '}'
				group_exception = false;
			}
		}
	}

	return new TestGroup(name, tests, gm, gs, gt);
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
	TestGroup* group = ParseTestGroup();
	return new TestFile(group);
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

InitializerValue::~InitializerValue() {
	if (mArgValue) delete mArgValue;
	if (mStructValue) delete mStructValue;
}

void InitializerValue::dump() {
	if (mArgValue) mArgValue->dump();
	if (mStructValue) mStructValue->dump();
}

void InitializerValue::accept(Visitor *v) {
	if (mArgValue) mArgValue->accept(v);
	if (mStructValue) mStructValue->accept(v);
	v->VisitInitializerValue(this);
}