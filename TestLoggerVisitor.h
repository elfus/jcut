/*
 * File:   TestLoggerVisitor.h
 * Author: aortegag
 *
 * Created on May 29, 2014, 11:00 AM
 */

#ifndef TESTLOGGERVISITOR_H
#define	TESTLOGGERVISITOR_H

#include <iostream>
#include <iomanip>
#include "Visitor.h"
#include "TestParser.h"

using namespace std;
using namespace tp;

/**
 * Logs the results of the tests ran in standard output
 */
class TestLoggerVisitor : public Visitor {
private:
    unsigned WIDTH = 80;
    unsigned TN_WIDTH = 20; // TEST NAME WIDTH
    unsigned RESULT_WIDTH = 8;
    unsigned EXP_WIDTH = 20;
public:
    TestLoggerVisitor() {}
    ~TestLoggerVisitor() {}

    void VisitTestFileFirst(TestFile* TF)
    {
        cout << setw(WIDTH) << setfill('=') << '=' << setfill(' ') << endl;
        cout << setw(TN_WIDTH) << left << "Test name";
        cout << setw(RESULT_WIDTH) << "Result";
        cout << setw(EXP_WIDTH) << "Expected result"<<endl;
        cout << setw(WIDTH) << setfill('-') << '-' << setfill(' ') << endl;
    }


    void VisitTestDefinition(TestDefinition *TD) {
        cout << setw(TN_WIDTH) << TD->getTestName();
        cout << setw(RESULT_WIDTH) << 
                (TD->getReturnValue().IntVal.getBoolValue()?
                "PASSED" : "FAILED");

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
            cout << setw(EXP_WIDTH) << ss.str() << endl;
        }
    }
};

#endif	/* TESTLOGGERVISITOR_H */

