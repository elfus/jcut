//===-- jcut/TestLoggerVisitor.h - Logging facilities -----------*- C++ -*-===//
//
// Copyright (c) 2014 Adrián Ortega García
// All rights reserved.
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
    enum LogFormat {
        LOG_ALL = 1 << 0, // 1
        LOG_PASSING = 1 << 1, // 2
        LOG_FAILING = 1 << 2, // 4
        LOG_TEST_SETUP = 1 << 3, // 8
        LOG_TEST_TEARDOWN = 1 << 4, // 16
        LOG_TEST_CLEANUP = 1 << 5,
        LOG_GROUP_SETUP = 1 << 6,
        LOG_GROUP_TEARDOWN = 1 << 7,
        LOG_GROUP_CLEANUP = 1 << 8,
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
    int mFmt = LOG_ALL;
    bool mCurrentTestPassed = false;

    void logFunctionOutput(LLVMFunctionHolder *FH, const string& name);
public:

    TestLoggerVisitor() {
        mPadding = " | ";
        /////////////////////////////////////////
        // These ones have to be maintained manually.
        mColumnName[GROUP_NAME] = "GROUP NAME";
        mColumnName[TEST_NAME] = "TEST NAME";
        mColumnName[FUD] = "FUNCTION CALLED";
        mColumnName[RESULT] = "RESULT";
        mColumnName[ACTUAL_RESULT] = "ACTUAL RESULT";
        mColumnName[EXPECTED_RES] = "EXPECTED RESULT";
        mColumnName[WARNING] = "WARNING";
        mColumnName[FUD_OUTPUT] = "FUNCTION OUTPUT";
        mColumnName[FAILED_EE] = "FAILED EXPECTED EXPRESSIONS";
        /////////////////////////////////////////

        for(auto& it : mColumnName)
            mColumnWidth[it.first] = it.second.size();

        // The order in which we will print the columns.
        // @todo load these ones dynamically
        mOrder.push_back(GROUP_NAME);
        mOrder.push_back(TEST_NAME);
        mOrder.push_back(FUD);
        mOrder.push_back(RESULT);
        mOrder.push_back(ACTUAL_RESULT);
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

    void VisitGroupSetup(GlobalSetup *GS) {
        if(mFmt & (LOG_GROUP_SETUP)) {
            cout << setw(WIDTH) << setfill('<') << '<' << setfill(' ') << endl;
            logFunctionOutput(GS, GS->getLLVMFunction()->getName());
            cout << setw(WIDTH) << setfill('.') << '.' << setfill(' ') << endl;
            cout << setw(WIDTH) << setfill('<') << '<' << setfill(' ') << endl;
        }
    }

    void VisitTestDefinitionFirst(TestDefinition *TD) {
        // Print the columns in the given order, then print a new line and
		// optionally print more information about the current test.
		++mTestCount;
		const map<ColumnName,string>& results = TD->getTestResults();
		mCurrentTestPassed = (results.at(RESULT) == "PASSED")? true : false;
        if(mCurrentTestPassed && (mFmt & (LOG_ALL | LOG_PASSING)) ){
        	++mTestsPassed;
            for(auto column : mOrder)
            	cout << setw(mColumnWidth[column]) << results.at(column) << mPadding;
        }
        else if(mCurrentTestPassed==false && (mFmt & (LOG_ALL | LOG_FAILING)) ) {
        	++mTestsFailed;
            for(auto column : mOrder)
            	cout << setw(mColumnWidth[column]) << results.at(column) << mPadding;
        }
    }

    void VisitTestDefinition(TestDefinition *TD) {
    	const map<ColumnName,string>& results = TD->getTestResults();
    	cout << results.at(WARNING) << endl;
    	string test_output = results.at(FUD_OUTPUT);
    	if(test_output.size()) {
    		string fud = TD->getTestFunction()->getFunctionCall()->getFunctionCalledString();
			stringstream ss;
			ss << "[" << fud << " output]";
			cout << setw(WIDTH) << setfill('.') << '.' << setfill(' ') << endl;
			cout << setw(WIDTH) << right << ss.str() << left << endl;
			cout << test_output << endl;
		}
    	string failed_ee = results.at(FAILED_EE);
    	if(!failed_ee.empty())
    		cout << failed_ee;
        cout << setw(WIDTH) << setfill('-') << '-' << setfill(' ') << endl;
    }

    void VisitGroupTeardown(GlobalTeardown *GT) {
        if(mFmt & LOG_GROUP_TEARDOWN) {
            cout << setw(WIDTH) << setfill('>') << '>' << setfill(' ') << endl;
            logFunctionOutput(GT, GT->getLLVMFunction()->getName());
            cout << setw(WIDTH) << setfill('>') << '>' << setfill(' ') << endl;
        }
    }

    // Log the group cleanup
    void VisitTestGroup(TestGroup *TG) {
        if((mFmt & LOG_GROUP_CLEANUP) && TG->getLLVMFunction()) {
            cout << setw(WIDTH) << setfill('>') << '>' << setfill(' ') << endl;
            logFunctionOutput(TG, TG->getLLVMFunction()->getName());
            cout << setw(WIDTH) << setfill('>') << '>' << setfill(' ') << endl;
        }
    }

    /// @note In order for the new column width to take effect this method has
    /// to be called before visiting any of the nodes.
    void setColumnWidth(ColumnName column, unsigned width) {
        mColumnWidth[column] = width;
    }

    void setLogFormat(int fmt) { mFmt = fmt; }

    const vector<ColumnName>& getColumnOrder() { return mOrder; }
    const map<ColumnName,unsigned>& getColumnWidths() { return mColumnWidth; }
    const map<ColumnName,string>& getColumnNames() { return mColumnName; }

    unsigned getTestsFailed() const { return mTestsFailed; }

};

class OutputFixerVisitor : public Visitor {
    TestLoggerVisitor& mLogger;
    const vector<ColumnName>& mOrder;
    const map<ColumnName,unsigned>& mColumnWidth;
public:
    OutputFixerVisitor() = delete;
    OutputFixerVisitor(const OutputFixerVisitor&) = delete;
    OutputFixerVisitor& operator = (const OutputFixerVisitor& ) = delete;

    OutputFixerVisitor(TestLoggerVisitor& lv) : mLogger(lv),
    mOrder(mLogger.getColumnOrder()), mColumnWidth(mLogger.getColumnWidths()){
    }

    void VisitTestDefinition(TestDefinition *TD) {
        for(auto column : mOrder) {
            string str = TestResults::getColumnString(column, TD);
            if(str.size() > mColumnWidth.at(column))
                mLogger.setColumnWidth(column, str.size());
        }
    }

    virtual void VisitTestFile(TestFile*) {
        mLogger.calculateTotalWidth();
    }

};

#endif	/* TESTLOGGERVISITOR_H */

