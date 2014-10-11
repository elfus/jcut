//===-- jcut/TestLoggerVisitor.cpp - Logging facilities ---------*- C++ -*-===//
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
#include <TestLoggerVisitor.h>

string TestLoggerVisitor::getColumnString(ColumnName name, TestDefinition *TD)
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

string TestLoggerVisitor::getActualResultString(TestDefinition *TD)
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

string TestLoggerVisitor::getExpectedResultString(TestDefinition *TD)
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
