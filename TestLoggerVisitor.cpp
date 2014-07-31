#include <TestLoggerVisitor.h>

string TestLoggerVisitor::getColumnString(ColumnName name, TestDefinition *TD)
{
	switch(name) {
		case TEST_NAME:
			return TD->getTestName();
			break;
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
				ss<<"Invalid Constant!"<<endl;
				break;
		}
		return ss.str();
	}
	return "Invalid expected result";
}