// Copyright Glen Knowles 2018 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// exec.h - dim system
//
// Execution of another process, supports:
//  - Synchronous and/or asynchronous execution
//  - Child process or elevated child of the system shell
//  - Allows reading/writing to the standard in/out/err of the child
//      (for elevated children, requires cooperation)

#pragma once

#include "cppconf/cppconf.h"

#include "dimcli/cli.h"

#include "core/charbuf.h"
#include "core/task.h"
#include "core/time.h"

#include <chrono>
#include <map>
#include <string_view>

namespace Dim {

enum StdStream {
    kStdIn,
    kStdOut,
    kStdErr,
};


/****************************************************************************
*
*   Execute child process
*
***/

struct ExecResult {
    std::string cmdline;
    int exitCode = {};
    CharBuf out;
    CharBuf err;
};

struct ExecOptions {
    std::string workingDir;

    // This is a delta applied to the application's environment. Set variables
    // to the empty string to remove them.
    //
    // The working directory for one or more drives can be set using special
    // environment variables named after the drive. For example, setting "=C"
    // sets the C drive's working directory for the child process.
    //
    // NOTE: Window's docs are unclear, maybe it's "=C:" instead of "=C" ?
    //       Please remove this note when verified!
    std::map<std::string, std::string> envVars;

    Dim::Duration timeout = (std::chrono::minutes) 5;
    std::string stdinData;

    // Enables additional writes to stdin of child process after it has
    // been started.
    bool enableExecWrite = false;

    Dim::TaskQueueHandle hq;
    size_t concurrency = (size_t) -1;
};

//---------------------------------------------------------------------------
// Complex asynchronous
//---------------------------------------------------------------------------
class IExecNotify {
public:
    virtual ~IExecNotify() = default;

    virtual void onExecComplete(bool canceled, int exitCode) = 0;

    // Default implementation accumulates the output into m_out & m_err
    // Returning false stops reading stream
    virtual bool onExecRead(
        size_t * bytesUsed,
        StdStream strm, // kStdOut or kStdErr
        std::string_view data
    );

protected:
    CharBuf m_out;
    CharBuf m_err;

private:
    friend class ExecProgram;
    ExecProgram * m_exec = nullptr;
};

void execProgram(
    IExecNotify * notify,
    const std::string & cmdline,
    const ExecOptions & opts = {}
);
void execProgram(
    IExecNotify * notify,
    const std::vector<std::string> & args,
    const ExecOptions & opts = {}
);

//===========================================================================
template <typename ...Args>
void execProgram(
    IExecNotify * notify,
    const ExecOptions & opts,
    Args... args
) {
    auto vargs = Cli::toArgvL(args...);
    execProgram(notify, vargs, opts);
}

// Write to standard input of child process. execProgram() must have been
// called for the notify with the enableExecWrite option set to true.
// WARNING: not tested
void execWrite(IExecNotify * notify, std::string_view data);

void execCancel(IExecNotify * notify);

//---------------------------------------------------------------------------
// Simple asynchronous
//---------------------------------------------------------------------------
void execProgram(
    std::function<void(ExecResult && res)> fn,
    const std::string & cmdline,
    const ExecOptions & opts = {}
);
void execProgram(
    std::function<void(ExecResult && res)> fn,
    const std::vector<std::string> & args,
    const ExecOptions & opts = {}
);

//===========================================================================
template <typename ...Args>
void execProgram(
    std::function<void(ExecResult && res)> fn,
    const ExecOptions & opts,
    Args... args
) {
    auto vargs = Cli::toArgvL(args...);
    execProgram(fn, vargs, opts);
}

//---------------------------------------------------------------------------
// Synchronous
//---------------------------------------------------------------------------
bool execProgramWait(
    ExecResult * res,
    const std::string & cmdline,
    const ExecOptions & opts = {}
);
bool execProgramWait(
    ExecResult * res,
    const std::vector<std::string> & args,
    const ExecOptions & opts = {}
);

//===========================================================================
template <typename... Args>
bool execProgramWait(
    ExecResult * res,
    const ExecOptions & opts,
    Args... args
) {
    auto vargs = Cli::toArgvL(args...);
    return execProgramWait(res, vargs, opts);
}


/****************************************************************************
*
*   Elevated process
*
***/

// Waits for program to complete, returns false and sets exitCode to -1 if
// unable to execute program. Stdio is not captured. Not allowed from the event
// thread.
bool execElevatedWait(
    int * exitCode,
    const std::string & cmdline,
    const std::string & workingDir = {}
);

// Helpers to allow elevated process to manually attach it's stdio to the
// process that requested it's execution. Useful when a process is relaunching
// itself with elevated rights.
void execListen(
    IExecNotify * notify,
    std::string * pipeName,
    std::string * secret
);
void execClientAttach(
    const std::string & pipeName,
    const std::string & secret
);


/****************************************************************************
*
*   Other functions
*
***/

void execCancelWaiting();

} // namespace
