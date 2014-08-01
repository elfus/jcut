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
#include <map>
#include "Visitor.h"
#include "TestParser.h"
#include "TestGeneratorVisitor.h"

using namespace std;
using namespace tp;

/**
 * Logs the results of the tests ran in standard output
 */
class TestLoggerVisitor : public Visitor {
public:
    enum ColumnName {
        TEST_NAME = 0,
        RESULT,
        EXPECTED_RES,
        WARNING,
        MAX_COLUMN
    };
private:
    unsigned WIDTH = 80;
    unsigned TN_WIDTH = 20; // TEST NAME WIDTH
    unsigned RESULT_WIDTH = 8;
    unsigned EXP_WIDTH = 20;
    unsigned WARN_WIDTH = 30;
    // Column Name and its width
    map<ColumnName,unsigned> mColumnWidth;
    map<ColumnName,string> mColumnName;
    vector<ColumnName> mOrder;

    string getColumnString(ColumnName name, TestDefinition *TD);
    string getExpectedResultString(TestDefinition *TD);
    string getWarningString(TestDefinition *TD);
public:

    TestLoggerVisitor() {
        /////////////////////////////////////////
        // These ones have to be maintained manually.
        mColumnWidth[TEST_NAME] = TN_WIDTH;
        mColumnWidth[RESULT] = RESULT_WIDTH;
        mColumnWidth[EXPECTED_RES] = EXP_WIDTH;
        mColumnName[TEST_NAME] = "Test name";
        mColumnName[RESULT] = "Result";
        mColumnName[EXPECTED_RES] = "Expected result";
        /////////////////////////////////////////

        // The order in which we will print the columns.
        mOrder.push_back(TEST_NAME);
        mOrder.push_back(RESULT);
        mOrder.push_back(EXPECTED_RES);
    }
    TestLoggerVisitor(const TestLoggerVisitor&) = delete;
    TestLoggerVisitor& operator=(const TestGeneratorVisitor&) = delete;
    ~TestLoggerVisitor() {}

    void VisitTestFileFirst(TestFile* TF)
    {
        cout << left;
        cout << setw(WIDTH) << setfill('=') << '=' << setfill(' ') << endl;
        for(auto column : mOrder)
            cout << setw(mColumnWidth[column]) << mColumnName[column];
        cout << endl;
        cout << setw(WIDTH) << setfill('-') << '-' << setfill(' ') << endl;
    }


    void VisitTestDefinition(TestDefinition *TD) {
        for(auto column : mOrder)
            cout << setw(mColumnWidth[column]) << getColumnString(column, TD);
        cout << endl;// << setw(WIDTH) << setfill('-') << '-' << setfill(' ') << endl;
    }

    /// @note In order for the new column width to take effect this method has
    /// to be called before visiting any of the nodes.
    void setColumnWidth(ColumnName column, unsigned width) {
        mColumnWidth[column] = width;
    }

    void enableWarningColumn() {
        mColumnWidth[WARNING] = WARN_WIDTH;
        mColumnName[WARNING] = "Warning";
        mOrder.push_back(WARNING);
    }
};

#endif	/* TESTLOGGERVISITOR_H */

