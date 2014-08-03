#include <TestLoggerVisitor.h>

string TestLoggerVisitor::getColumnString(ColumnName name, TestDefinition *TD)
{
	switch(name) {
		case GROUP_NAME:
			return TD->getGroupName();
		case TEST_NAME:
			return TD->getTestName();
			break;
		case FUD:
			return TD->getTestFunction()->getFunctionCall()->getFunctionCalledString();
		case RESULT:
			return (TD->getReturnValue().IntVal.getBoolValue()?"PASSED" : "FAILED");
		case EXPECTED_RES:
			return getExpectedResultString(TD);
		default:
			return "Invalid column";
	}
	return "Invalid column2";
}

string TestLoggerVisitor::getExpectedResultString(TestDefinition *TD)
{
	// @todo Get the actual outcome from the call function
	// @todo detect what failed, after conditions?
	ExpectedResult* ER = TD->getTestFunction()->getExpectedResult();
	if (ER) {
		stringstream ss;
		Constant* C = ER->getExpectedConstant()->getConstant();
		ss << ER->getComparisonOperator()->getTypeStr() << " ";
		switch(C->getType()){
			case Constant::Type::NUMERIC:
			{
				NumericConstant* nc = C->getNumericConstant();
				if(nc->isFloat())
					ss << nc->getFloat();
				if(nc->isInt())
					ss << nc->getInt();
			}
				break;
			case Constant::Type::STRING:
			{

			}
				break;
			case Constant::Type::CHAR:
			{

			}
				break;
			default:
				ss<<"Invalid Constant!";
				break;
		}
		return ss.str();
	}
	return "Invalid expected result";
}

string TestLoggerVisitor::getWarningString(TestDefinition *TD)
{
	const vector<Exception>& warnings = TD->getWarnings();
	if(warnings.size()) {
		stringstream ss;
		ss << "[" << warnings.size() << "] ";
		for(auto w : warnings)
			ss << "[" << w.what() << "] ";
		return ss.str();
	}
	return "";
}

void TestLoggerVisitor::logTest(TestDefinition* TD)
{
	// Print the columns in the given order, then print a new line and
	// optionally print more information about the current test.
	for(auto column : mOrder)
		cout << setw(mColumnWidth[column]) << getColumnString(column, TD) << mPadding;
	cout << endl;
	const vector<Exception>& warnings = TD->getWarnings();
	if(warnings.size()) {
		for(auto w : warnings)
			cout << w.what() << endl;
	}
	// If we want to print more information about a test, this is the place
	// for example we want print its output.
	if(TD->getTestOutput().size()) {
		cout << setw(WIDTH) << setfill('.') << '.' << setfill(' ') << endl;
		cout << setw(WIDTH) << right << "[Test output]" << left << endl;
		cout << TD->getTestOutput() << endl;
	}
	cout << setw(WIDTH) << setfill('-') << '-' << setfill(' ') << endl;
}