// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// exec.h - dim system
#pragma once

#include "cppconf/cppconf.h"

#include "core/core.h"
#include "file/file.h"

#include <string_view>

namespace Dim {

enum StdStream {
    kStdIn,
    kStdOut,
    kStdErr,
};

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
    ExecProgram * m_exec;
};


void execProgram(
    IExecNotify * notify,
    std::string_view prog,
    std::string_view args
);

// Write to stdin of child process
void execWrite(IExecNotify * notify, std::string_view data);

void execCancel(IExecNotify * notify);


void execListen(
    IExecNotify * notify,
    std::string * pipeName,
    std::string * secret
);
void execClientAttach(std::string_view pipeName, std::string_view secret);


bool execProgramWait(
    int * exitCode,
    CharBuf * out,
    CharBuf * err,
    std::string_view prog,
    std::string_view args
);

// Waits for program to complete and returns its exit code, returns -1
// if unable to execute program.
bool execElevated(int * exitCode, std::string_view prog, std::string_view args);

} // namespace
