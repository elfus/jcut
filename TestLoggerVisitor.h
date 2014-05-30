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
#include "Visitor.hxx"
#include "TestParser.hxx"

using namespace std;
using namespace tp;

/**
 * Logs the results of the tests ran in standard output
 */
class TestLoggerVisitor : public Visitor {
public:
    TestLoggerVisitor() {}
    ~TestLoggerVisitor() {}

    void VisitUnitTestFirst(UnitTestExpr* UT)
    {   cout << setw(60) << setfill('=') << '=' << endl;
        cout << "Test name\t\tResult\tExpected result"<<endl;
        cout << setw(60) << setfill('-') << '-' << endl;
    }


    void VisitTestDefinitionExpr(TestDefinitionExpr *TD) {
        cout << TD->getTestName() << "\t\t";
        cout << (TD->getReturnValue().IntVal.getBoolValue()?
                "PASSED" : "FAILED") << "\t";
        // @todo Get the actual outcome from the call function
        // @todo detect what failed, after conditions?
        ExpectedResult* ER = TD->getTestFunction()->getExpectedResult();
        if (ER) {
            cout << ER->getComparisonOperator()->getTypeStr() << " "<<ER->getExpectedConstant()->getValue();
        }
        cout << endl;
    }
};

#endif	/* TESTLOGGERVISITOR_H */

