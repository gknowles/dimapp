// Copyright Glen Knowles 2021 - 2022.
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
    function<void(ExecToolResult&&)> fn,
    string_view cmdline,
    string_view errTitle,
    const ExecOptions & opts,
    const vector<int> & codes
) {
    struct Exec : IExecNotify {
        function<void(ExecToolResult&&)> fn;
        string cmdline;
        string errTitle;
        vector<int> codes;

        void onExecComplete(
            ExecResult::Type exitType,
            int exitCode
        ) override {
            ExecToolResult res = {};
            if (exitType == ExecResult::kNotStarted
                || exitType == ExecResult::kTimeout
            ) {
                // Rely on execProgram to have already logged an error.
                logMsgError() << "Error: " << errTitle;
                logMsgInfo() << " - " << cmdline;
                if (exitType == ExecResult::kNotStarted) {
                    logMsgInfo() << " - program not started.";
                } else {
                    assert(exitType == ExecResult::kTimeout);
                    logMsgInfo() << " - program timeout exceeded.";
                }
            } else if (exitType == ExecResult::kCanceled) {
                assert(appMode() == kRunStopping);
                res.success = true;
            } else if (
                find(codes.begin(), codes.end(), exitCode) == codes.end()
            ) {
                logMsgError() << "Error: " << errTitle;
                logMsgInfo() << " - " << cmdline;
                logMsgWarn() << " - exit code: " << exitCode;
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
            } else {
                if (m_err) {
                    logMsgWarn() << "Warn: " << errTitle;
                    logMsgWarn() << " - " << cmdline;
                    logMsgWarn() << " - " << m_err;
                }
                res.success = true;
                res.output = move(toString(m_out));
            }
            if (!res.success)
                appSignalShutdown(EX_IOERR);
            fn(move(res));
            delete this;
        }
    };
    auto notify = new Exec;
    notify->fn = fn;
    notify->cmdline = cmdline;
    notify->errTitle = errTitle;
    notify->codes = codes;
    auto tmpOpts = opts;
    if (tmpOpts.concurrency == -1)
        tmpOpts.concurrency = envProcessors();
    execProgram(notify, notify->cmdline, tmpOpts);
}

//===========================================================================
ExecToolResult Dim::execToolWait(
    string_view cmdline,
    string_view errTitle,
    const ExecOptions & opts,
    const vector<int> & codes
) {
    ExecToolResult out;
    latch lat(1);
    auto tmpOpts = opts;
    tmpOpts.hq = taskInEventThread()
        ? taskComputeQueue()
        : taskEventQueue();
    execTool(
        [&](auto && content){
            out = move(content);
            lat.count_down();
        },
        cmdline,
        errTitle,
        tmpOpts,
        codes
    );
    lat.wait();
    return out;
}
