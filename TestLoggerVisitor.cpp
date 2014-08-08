#include <TestLoggerVisitor.h>

string TestLoggerVisitor::getColumnString(ColumnName name, TestFunction *TF)
{
	switch(name) {
		case GROUP_NAME:
			return TF->getGroupName();
		case TEST_NAME:
			return TF->getLLVMFunction()->getName();
		case FUD:
			return TF->getFunctionCall()->getFunctionCalledString();
        case RESULT:
			/// @todo Detect individually the conditions of the teardown functions
			return (TF->getReturnValue().IntVal.getBoolValue()?"PASSED" : "FAILED");
		case EXPECTED_RES:
			return getExpectedResultString(TF);
		default:
			return "Invalid column";
	}
	return "Invalid column2";
}

string TestLoggerVisitor::getExpectedResultString(TestFunction *TF)
{
	// @todo Get the actual outcome from the call function
	// @todo detect what failed, after conditions?
	ExpectedResult* ER = TF->getExpectedResult();
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

string TestLoggerVisitor::getWarningString(TestFunction *TF)
{
	const vector<Exception>& warnings = TF->getWarnings();
	if(warnings.size()) {
		stringstream ss;
		ss << "[" << warnings.size() << "] ";
		for(auto w : warnings)
			ss << "[" << w.what() << "] ";
		return ss.str();
	}
	return "";
}

void TestLoggerVisitor::logFunction(LLVMFunctionHolder *FH, const string& name)
{
	const vector<Exception>& warnings = FH->getWarnings();
	if(warnings.size()) {
		for(auto w : warnings)
			cout << w.what() << endl;
	}
	// If we want to print more information about a test, this is the place
	// for example we want print its output.
	if(FH->getOutput().size()) {
                stringstream ss;
                ss << "[" << name << " output]";
		cout << setw(WIDTH) << setfill('.') << '.' << setfill(' ') << endl;
		cout << setw(WIDTH) << right << ss.str() << left << endl;
		cout << FH->getOutput() << endl;
	}
}