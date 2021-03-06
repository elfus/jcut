//===-- jcut/OSRedirect.h - OSRedirect class definition ---------*- C++ -*-===//
//
// This file is part of JCUT, A Just-n-time C Unit Testing framework.
//
// Copyright (c) 2014 Adrián Ortega García <adrianog(dot)sw(at)gmail(dot)com>
// All rights reserved.
//
// JCUT is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// JCUT is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with JCUT (See LICENSE.TXT for details).
// If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//

#ifndef OSREDIRECT_H
#define	OSREDIRECT_H

#include <iostream>
#include <sstream>

using namespace std;

/// Redirects an output stream to a string object
class OSRedirect {
private:
    ostream& mOriginalStrm;
    streambuf* mOldStrmBuf;
    stringstream ss;
public:
    OSRedirect(ostream& strm) : mOriginalStrm(strm), mOldStrmBuf(nullptr) {
        mOldStrmBuf = mOriginalStrm.rdbuf(ss.rdbuf());
    }

    ~OSRedirect() {
        mOriginalStrm.rdbuf(mOldStrmBuf);
    }

    OSRedirect(const OSRedirect&) = delete;
    OSRedirect& operator=(const OSRedirect&) = delete;

    string getOutput() const {
        return ss.str();
    }
};

/// The following code was retrieved from
/// http://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string
/// From user Sir Digby Chicken Caesar
/// The credit of the following code goes to that user!
/// I am not the author of the StdCapture class..

// For some reason Mingw32 did not defined this one, but I'll leave it here
// for future reference
// #ifdef _MSC_VER
#ifdef __MINGW32__
#include <io.h>
#define popen _popen
#define pclose _pclose
#define stat _stat
#define dup _dup
#define dup2 _dup2
#define fileno _fileno
#define close _close
#define pipe _pipe
#define read _read
#define eof _eof
#define MSG "WINDOWS!"
#else
#include <unistd.h>
#define MSG "LINUX!"
#endif
#include <fcntl.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <mutex>

class StdCapture
{
public:
    static bool BeginCapture()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

// #ifdef _MSC_VER
#ifdef __MINGW32__
        if (pipe(m_pipe, 65536, O_BINARY) == -1)
#else
        if (pipe(m_pipe) == -1)
#endif
            return false;

        m_oldStdOut = dup(fileno(stdout));
        m_oldStdErr = dup(fileno(stderr));
        if (m_oldStdOut == -1 || m_oldStdErr == -1)
        {
            cleanup_fds();
            return false;
        }

        if (m_capturing)
            EndCapture();

        fflush(stdout);
        fflush(stderr);
        dup2(m_pipe[WRITE], fileno(stdout));
        dup2(m_pipe[WRITE], fileno(stderr));
        m_capturing = true;
        return true;
    }
    static bool IsCapturing()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_capturing;
    }
    static bool EndCapture()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_capturing)
            return false;

        fflush(stdout);
        fflush(stderr);
        dup2(m_oldStdOut, fileno(stdout));
        dup2(m_oldStdErr, fileno(stderr));
        m_captured.clear();
        close(m_pipe[WRITE]); // write pipe fd must be closed in linux in order to read from read fd
        m_pipe[WRITE] = 0;

        const int bufSize = 1024;
        char buf[bufSize];
        int bytesRead = 0;
        do
        {
//#ifdef _MSC_VER
#ifdef __MINGW32__
            bytesRead = 0;
            if (!eof(m_pipe[READ]))
                bytesRead = read(m_pipe[READ], buf, bufSize); // windows can't handle read if file descriptor is at end of file
#else
            bytesRead = read(m_pipe[READ], buf, bufSize); // linux has no eof file function as read can handle aformentioned case
#endif
            if (bytesRead > 0)
            {
                buf[bytesRead] = 0;
                m_captured += buf;
            }
        }
        while(bytesRead == bufSize);

        m_capturing = false;
        cleanup_fds();
        return true;
    }
    static std::string GetCapture()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_captured;
    }
private:
    enum PIPES { READ, WRITE };
    static void cleanup_fds()
    {
        if (m_oldStdOut > 0)
            close(m_oldStdOut);
        if (m_oldStdErr > 0)
            close(m_oldStdErr);
        if (m_pipe[READ] > 0)
            close(m_pipe[READ]);
        if (m_pipe[WRITE] > 0)
            close(m_pipe[WRITE]);

        m_oldStdOut = 0;
        m_oldStdErr = 0;
        m_pipe[READ] = 0;
        m_pipe[WRITE] = 0;
    }

    static int m_pipe[2];
    static int m_oldStdOut;
    static int m_oldStdErr;
    static bool m_capturing;
    static std::mutex m_mutex;
    static std::string m_captured;
};


#endif	/* OSREDIRECT_H */

