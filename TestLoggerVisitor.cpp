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
			return (TF->getPassingValue()?"PASSED" : "FAILED");
		case ACTUAL_RESULT:
			return getActualResultString(TF);
		case EXPECTED_RES:
			return getExpectedResultString(TF);
		default:
			return "Invalid column";
	}
	return "Invalid column2";
}

string TestLoggerVisitor::getActualResultString(TestFunction *TF)
{
	llvm::GenericValue result(TF->getReturnValue());
	stringstream ss;
	llvm::Type *returnType = TF->getFunctionCall()->getReturnType();
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
				const string& s = C->getStringConstant()->getString();
				stringstream tmp;
				tmp << static_cast<const void*>(s.c_str());
				unsigned long addr =  stoul(tmp.str(), nullptr, 16);
				ss << "0x" << std::hex << addr << " (" << s << ")";
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