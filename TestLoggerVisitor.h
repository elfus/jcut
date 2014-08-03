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
        GROUP_NAME = 0,
        TEST_NAME,
        FUD, // Function Under Test
        RESULT,
        EXPECTED_RES,
        WARNING,
        MAX_COLUMN
    };

    enum LogFormat {
        LOG_ALL = 1 << 0, // 1
        LOG_PASSING = 1 << 1, // 2
        LOG_FAILING = 1 << 2, // 4
    };
private:
    unsigned WIDTH; // The 'terminal' width
    // Column Name and its width
    map<ColumnName,unsigned> mColumnWidth;
    map<ColumnName,string> mColumnName;
    vector<ColumnName> mOrder;
    string mPadding;
    unsigned mTestCount = 0;
    unsigned mTestsPassed = 0;
    unsigned mTestsFailed = 0;
    LogFormat mFmt = LOG_ALL;

    string getExpectedResultString(TestDefinition *TD);
    string getWarningString(TestDefinition *TD);
    void logTest(TestDefinition *TD);
public:

    TestLoggerVisitor() {
        mPadding = " | ";
        /////////////////////////////////////////
        // These ones have to be maintained manually.
        mColumnName[GROUP_NAME] = "GROUP NAME";
        mColumnName[TEST_NAME] = "TEST NAME";
        mColumnName[FUD] = "FUNCTION CALLED";
        mColumnName[RESULT] = "RESULT";
        mColumnName[EXPECTED_RES] = "EXPECTED RESULT";
        /////////////////////////////////////////

        for(auto& it : mColumnName)
            mColumnWidth[it.first] = it.second.size();

        // The order in which we will print the columns.
        // @todo load these ones dynamically
        mOrder.push_back(GROUP_NAME);
        mOrder.push_back(TEST_NAME);
        mOrder.push_back(FUD);
        mOrder.push_back(RESULT);
        mOrder.push_back(EXPECTED_RES);

        calculateTotalWidth();
    }
    TestLoggerVisitor(const TestLoggerVisitor&) = delete;
    TestLoggerVisitor& operator=(const TestGeneratorVisitor&) = delete;
    ~TestLoggerVisitor() {}

    void calculateTotalWidth() {
        WIDTH = 0;
        for(auto column : mOrder)
            WIDTH += mColumnWidth[column];
        WIDTH += mPadding.size()*mOrder.size();// Add the padding for each column
    }

    void VisitTestFileFirst(TestFile* TF)
    {
        cout << left;
        cout << setw(WIDTH) << setfill('=') << '=' << setfill(' ') << endl;
        for(auto column : mOrder)
            cout << setw(mColumnWidth[column]) << mColumnName[column] << mPadding;
        cout << endl;
        cout << setw(WIDTH) << setfill('~') << '~' << setfill(' ') << endl;
    }

    void VisitTestFile(TestFile* TF)
    {
        assert(mTestCount == (mTestsPassed + mTestsFailed) && "Invalid test count");
        cout << setw(WIDTH) << setfill('~') << '~' << setfill(' ') << endl;
        cout << "TEST SUMMARY" << endl;
        cout << "Tests ran: " << mTestCount << endl;
        cout << "Tests PASSED: " << mTestsPassed << endl;
        cout << "Tests FAILED: " << mTestsFailed << endl;
    }


    void VisitTestDefinition(TestDefinition *TD) {
        ++mTestCount;
        if(TD->getReturnValue().IntVal.getBoolValue()) {
            ++mTestsPassed;
            if(mFmt & LOG_ALL || mFmt & LOG_PASSING)
                logTest(TD);
        }
        else {
            ++mTestsFailed;
            if(mFmt & LOG_ALL || mFmt & LOG_FAILING)
                logTest(TD);
        }
    }

    /// @note In order for the new column width to take effect this method has
    /// to be called before visiting any of the nodes.
    void setColumnWidth(ColumnName column, unsigned width) {
        mColumnWidth[column] = width;
    }

    void setLogFormat(LogFormat fmt) { mFmt = fmt; }

    const vector<ColumnName>& getColumnOrder() { return mOrder; }
    const map<ColumnName,unsigned>& getColumnWidths() { return mColumnWidth; }
    string getColumnString(ColumnName name, TestDefinition *TD);

    unsigned getTestsFailed() const { return mTestsFailed; }

};

class OutputFixerVisitor : public Visitor {
    TestLoggerVisitor& mLogger;
    const vector<TestLoggerVisitor::ColumnName>& mOrder;
    const map<TestLoggerVisitor::ColumnName,unsigned>& mColumnWidth;
public:
    OutputFixerVisitor() = delete;
    OutputFixerVisitor(const OutputFixerVisitor&) = delete;
    OutputFixerVisitor& operator = (const OutputFixerVisitor& ) = delete;

    OutputFixerVisitor(TestLoggerVisitor& lv) : mLogger(lv),
    mOrder(mLogger.getColumnOrder()), mColumnWidth(mLogger.getColumnWidths()){
    }

    void VisitTestDefinition(TestDefinition *TD) {
        for(auto column : mOrder) {
            string str = mLogger.getColumnString(column, TD);
            if(str.size() > mColumnWidth.at(column))
                mLogger.setColumnWidth(column, str.size()); // +1 whitespace
        }
    }

    virtual void VisitTestFile(TestFile*) {
        mLogger.calculateTotalWidth();
    }

};

#endif	/* TESTLOGGERVISITOR_H */

