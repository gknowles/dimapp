// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winres.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace Dim {

class ExecProgram : public IWinEventWaitNotify {
public:
    class ExecPipe : public IPipeNotify {
    public:
        // Inherited via IPipeNotify
        bool onPipeRead(size_t * bytesUsed, string_view data) override;
        void onPipeDisconnect() override;

        ExecProgram * m_notify{nullptr};
        StdStream m_strm;
        bool m_closed{false};
    };

public:
    ExecProgram(IExecNotify * notify);
    ~ExecProgram();
    void exec(string_view prog, string_view args);

    bool onRead(size_t * bytesUsed, StdStream strm, string_view data);
    void onDisconnect(StdStream strm);

    // Inherited via IWinEventWaitNotify
    // Triggered when child process exits
    void onTask() override;

private:
    bool createPipe(
        HANDLE * hchild,
        StdStream strm,
        const Path & name,
        Pipe::OpenMode oflags
    );
    void checkIfDone();

    IExecNotify * m_notify{nullptr};
    ExecPipe m_pipes[3];
    unsigned m_connected{0};
    HANDLE m_process;

    char m_outbuf[4096];
    char m_errbuf[4096];

    RunMode m_mode{kRunStarting};
    bool m_canceled{false};
    int m_exitCode{0};
};

} // namespace


/****************************************************************************
*
*   ExecProgram::ExecPipe
*
***/

//===========================================================================
bool ExecProgram::ExecPipe::onPipeRead(
    size_t * bytesUsed,
    std::string_view data
) {
    return m_notify->onRead(bytesUsed, m_strm, data);
}

//===========================================================================
void ExecProgram::ExecPipe::onPipeDisconnect() {
    m_notify->onDisconnect(m_strm);
}


/****************************************************************************
*
*   ExecProgram
*
***/

//===========================================================================
ExecProgram::ExecProgram(IExecNotify * notify)
    : m_notify(notify)
{}

//===========================================================================
ExecProgram::~ExecProgram() {
    for (auto && pi : m_pipes)
        pipeClose(&pi);
    CloseHandle(m_process);
    if (m_notify)
        m_notify->m_exec = nullptr;
}

//===========================================================================
void ExecProgram::exec(string_view prog, string_view args) {
    char name[100];
    snprintf(
        name,
        sizeof(name),
        "//./pipe/local/%u_%p",
        envProcessId(),
        this
    );

    struct {
        StdStream strm;
        Path name;
        Pipe::OpenMode oflags;
        HANDLE child;
    } pipes[] = {
        { kStdIn, Path(name).setExt("in"), Pipe::fWriteOnly },
        { kStdOut, Path(name).setExt("out"), Pipe::fReadOnly },
        { kStdErr, Path(name).setExt("err"), Pipe::fReadOnly },
    };
    for (auto && p : pipes) {
        if (!createPipe(&p.child, p.strm, p.name, p.oflags)) {
            for (auto && p2 : pipes) {
                if (p2.child)
                    CloseHandle(p2.child);
            }
            return m_notify->onExecComplete(true, -1);
        }
    }

    auto wprog = toWstring(prog);
    auto wargs = toWstring(args);
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = pipes[kStdIn].child;
    si.hStdOutput = pipes[kStdOut].child;
    si.hStdError = pipes[kStdErr].child;
    PROCESS_INFORMATION pi;
    bool running = CreateProcessW(
        wprog.c_str(),
        wargs.data(),
        NULL,   // process attrs
        NULL,   // thread attrs
        true,
        CREATE_NEW_PROCESS_GROUP,
        NULL, // environment
        NULL, // current dir
        &si,
        &pi
    );
    for (auto && p : pipes)
        CloseHandle(p.child);
    if (!running)
        return m_notify->onExecComplete(true, -1);

    m_process = pi.hProcess;
    registerWait(pi.hProcess);
}

//===========================================================================
bool ExecProgram::onRead(
    size_t * bytesUsed,
    StdStream strm,
    string_view data
) {
    return m_notify->onExecRead(bytesUsed, strm, data);
}

//===========================================================================
void ExecProgram::onDisconnect(StdStream strm) {
    auto & pi = m_pipes[strm];
    pi.m_closed = true;
    checkIfDone();
}

//===========================================================================
void ExecProgram::onTask() {
    m_mode = kRunStopped;
    DWORD rc;
    if (GetExitCodeProcess(m_process, &rc)) {
        m_canceled = false;
        m_exitCode = rc;
    } else {
        m_canceled = true;
        m_exitCode = -1;
    }
    checkIfDone();
}

//===========================================================================
void ExecProgram::checkIfDone() {
    if (m_pipes[kStdOut].m_closed
        && m_pipes[kStdErr].m_closed
        && m_mode == kRunStopped
    ) {
        m_notify->onExecComplete(m_canceled, m_exitCode);
        delete this;
    }
}

//===========================================================================
bool ExecProgram::createPipe(
    HANDLE * child,
    StdStream strm,
    const Path & name,
    Pipe::OpenMode oflags
) {
    pipeListen(&m_pipes[strm], name, oflags);

    if (child) {
        auto wname = toWstring(name);
        SECURITY_ATTRIBUTES sa = {};
        sa.bInheritHandle = true;
        auto cp = CreateFileW(
            wname.c_str(),
            oflags == Pipe::fReadOnly ? GENERIC_READ : GENERIC_WRITE,
            0,      // share
            &sa,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL    // template file
        );
        if (cp == INVALID_HANDLE_VALUE) {
            logMsgError() << "CreateFile(pipe): " << WinError{};
            return false;
        }
        *child = cp;
    }

    return true;
}


/****************************************************************************
*
*   IExecNotify
*
***/

//===========================================================================
bool IExecNotify::onExecRead(
    size_t * bytesUsed,
    StdStream strm,
    string_view data
) {
    switch (strm) {
    case kStdIn: break;
    case kStdOut: m_out.append(data); break;
    case kStdErr: m_err.append(data); break;
    }
    *bytesUsed = data.size();
    return true;
}


/****************************************************************************
*
*   Execute child program
*
***/

//===========================================================================
void Dim::execProgram(
    IExecNotify * notify,
    std::string_view prog,
    std::string_view args
) {
    assert(notify);

    char bytes[8];
    cryptRandomBytes(bytes, sizeof(bytes));

    auto ep = make_unique<ExecProgram>(notify);

}


/****************************************************************************
*
*   Execute child program and wait
*
***/

namespace {

class ExecWaitNotify : public IExecNotify {
public:
    void onExecComplete(bool canceled, int exitCode) override;

    bool wait(int * exitCode, CharBuf * out, CharBuf * err);

private:
    mutex m_mut;
    condition_variable m_cv;

    bool m_complete{false};
    bool m_canceled{false};
    int m_exitCode{0};
};

} // namespace

//===========================================================================
void ExecWaitNotify::onExecComplete(bool canceled, int exitCode) {
    m_canceled = canceled;
    m_exitCode = exitCode;
}

//===========================================================================
bool ExecWaitNotify::wait(int * exitCode, CharBuf * out, CharBuf * err) {
    unique_lock lk{m_mut};
    while (!m_complete)
        m_cv.wait(lk);
    *out = move(m_out);
    *err = move(m_err);
    *exitCode = m_exitCode;
    return !m_canceled;
}

//===========================================================================
bool Dim::execProgramWait(
    int * exitCode,
    CharBuf * out,
    CharBuf * err,
    std::string_view prog,
    std::string_view args
) {
    ExecWaitNotify notify;
    execProgram(&notify, prog, args);
    return notify.wait(exitCode, out, err);
}


/****************************************************************************
*
*   Elevated
*
***/

//===========================================================================
bool Dim::execElevated(int * exitCode, string_view prog, string_view args) {
    SHELLEXECUTEINFOW ei = { sizeof(ei) };
    ei.lpVerb = L"RunAs";
    auto wexe = toWstring(prog);
    ei.lpFile = wexe.c_str();
    auto wargs = toWstring(args);
    ei.lpParameters = wargs.c_str();
    ei.fMask = SEE_MASK_NOASYNC
        | SEE_MASK_NOCLOSEPROCESS
        | SEE_MASK_FLAG_NO_UI
        ;
    ei.nShow = SW_HIDE;
    if (!ShellExecuteExW(&ei)) {
        WinError err;
        if (err == ERROR_CANCELLED) {
            logMsgError() << "Operation canceled.";
        } else {
            logMsgError() << "ShellExecuteExW: " << WinError();
        }
        *exitCode = -1;
        return false;
    }

    DWORD rc = EX_OSERR;
    if (WAIT_OBJECT_0 == WaitForSingleObject(ei.hProcess, INFINITE))
        GetExitCodeProcess(ei.hProcess, &rc);
    *exitCode = rc;
    return true;
}
