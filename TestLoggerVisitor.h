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
public:
    TestLoggerVisitor() {}
    ~TestLoggerVisitor() {}

    void VisitTestFileFirst(TestFile* TF)
    {   cout << setw(60) << setfill('=') << '=' << endl;
        cout << "Test name\t\tResult\tExpected result"<<endl;
        cout << setw(60) << setfill('-') << '-' << endl;
    }


    void VisitTestDefinition(TestDefinition *TD) {
        cout << TD->getTestName() << "\t\t";
        cout << (TD->getReturnValue().IntVal.getBoolValue()?
                "PASSED" : "FAILED") << "\t";
        // @todo Get the actual outcome from the call function
        // @todo detect what failed, after conditions?
        ExpectedResult* ER = TD->getTestFunction()->getExpectedResult();
        if (ER) {
            Constant* C = ER->getExpectedConstant()->getConstant();
            cout << ER->getComparisonOperator()->getTypeStr() << " ";
            switch(C->getType()){
                case Constant::Type::NUMERIC:
                {
                    NumericConstant* nc = C->getNumericConstant();
                    if(nc->isFloat())
                        cout << nc->getFloat();
                    if(nc->isInt())
                        cout << nc->getInt();
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
                    cerr<<"Invalid Constant!"<<endl;
                    break;
            }
        }
        cout << endl;
    }
};

#endif	/* TESTLOGGERVISITOR_H */

