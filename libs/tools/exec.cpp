// Copyright Glen Knowles 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// exec.cpp - dim tools
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Execute utility subprocess
*
***/

//===========================================================================
void Dim::execTool(
    function<void(string&&)> fn,
    string_view cmdline,
    string_view errTitle,
    const ExecOptions & opts
) {
    struct Exec : IExecNotify {
        function<void(string&&)> fn;
        string cmdline;
        string errTitle;

        void onExecComplete(bool canceled, int exitCode) override {
            if (canceled) {
                // Rely on execProgram to have already logged an error.
                logMsgError() << "Error: " << errTitle;
                logMsgInfo() << " - " << cmdline;
                logMsgInfo() << " - execProgram timeout exceeded.";
                appSignalShutdown(EX_IOERR);
                fn({});
            } else if (exitCode) {
                logMsgError() << "Error: " << errTitle;
                logMsgInfo() << " - " << cmdline;
                vector<string_view> lines;
                auto tmp = toString(m_err);
                split(&lines, tmp, '\n');
                for (auto&& line : lines) {
                    line = rtrim(line);
                    if (line.size())
                        logMsgError() << " - " << line;
                }
                tmp = toString(m_out);
                split(&lines, tmp, '\n');
                for (auto&& line : lines) {
                    line = rtrim(line);
                    if (line.size())
                        logMsgInfo() << " - " << line;
                }
                appSignalShutdown(EX_IOERR);
                fn({});
            } else {
                if (m_err) {
                    logMsgWarn() << "Warn: " << errTitle;
                    logMsgWarn() << " - " << cmdline;
                    logMsgWarn() << " - " << m_err;
                }
                fn(toString(m_out));
            }
            delete this;
        }
    };
    auto notify = new Exec;
    notify->fn = fn;
    notify->cmdline = cmdline;
    notify->errTitle = errTitle;
    auto tmpOpts = opts;
    if (tmpOpts.concurrency == -1)
        tmpOpts.concurrency = envProcessors();
    execProgram(notify, notify->cmdline, tmpOpts);
}

//===========================================================================
string Dim::execToolWait(
    string_view cmdline,
    string_view errTitle,
    const ExecOptions & opts
) {
    string out;
    mutex mut;
    condition_variable cv;
    bool done = false;
    auto tmpOpts = opts;
    tmpOpts.hq = taskInEventThread()
        ? taskComputeQueue()
        : taskEventQueue();
    execTool(
        [&](auto && content){
            mut.lock();
            out = move(content);
            done = true;
            mut.unlock();
            cv.notify_one();
        },
        cmdline,
        errTitle,
        tmpOpts
    );
    unique_lock lk{mut};
    while (!done)
        cv.wait(lk);
    return out;
}
