// Copyright Glen Knowles 2019 - 2020.
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

class ExecProgram {
public:
    class ExecPipe : public IPipeNotify {
    public:
        // Inherited via IPipeNotify
        bool onPipeRead(size_t * bytesUsed, string_view data) override;
        void onPipeDisconnect() override;

        ExecProgram * m_notify = {};
        StdStream m_strm = {};
        bool m_closed = true;
    };

public:
    static void write(IExecNotify * notify, std::string_view data);

public:
    ExecProgram(IExecNotify * notify, TaskQueueHandle hq);
    ~ExecProgram();
    void exec(string_view cmdline, const ExecOptions & opts);
    TaskQueueHandle queue() const { return m_hq; }

    bool onRead(size_t * bytesUsed, StdStream strm, string_view data);
    void onDisconnect(StdStream strm);

    void onProcessExit();

private:
    bool createPipe(
        HANDLE * hchild,
        StdStream strm,
        string_view name,
        Pipe::OpenMode oflags
    );
    bool completeIfDone_LK();

    TaskQueueHandle m_hq;
    HANDLE m_job = NULL;
    HANDLE m_process = NULL;

    //-----------------------------------------------------------------------
    mutex m_mut;

    IExecNotify * m_notify{};
    ExecPipe m_pipes[3];
    unsigned m_connected{};

    RunMode m_mode{kRunStopped};
    bool m_canceled = true;
    int m_exitCode = -1;
};

} // namespace


/****************************************************************************
*
*   IOCP completion thread
*
***/

static mutex s_mut;
static HANDLE s_iocp;

static auto & s_perfPrograms = uperf("exec.programs");
static auto & s_perfCurPrograms = uperf("exec.programs (current)");

//===========================================================================
static void jobObjectIocpThread() {
    const int kMaxEntries = 8;
    OVERLAPPED_ENTRY entries[kMaxEntries];
    ULONG found;
    for (;;) {
        if (!GetQueuedCompletionStatusEx(
            s_iocp,
            entries,
            (ULONG) size(entries),
            &found,
            INFINITE,   // timeout
            false       // alertable
        )) {
            WinError err;
            if (err == ERROR_ABANDONED_WAIT_0) {
                // Completion port closed while inside get status.
                break;
            } else if (err == ERROR_INVALID_HANDLE) {
                // Completion port closed before call to get status.
                break;
            } else {
                logMsgFatal() << "GetQueuedCompletionStatusEx(JobPort): "
                    << err;
            }
        }

        for (unsigned i = 0; i < found; ++i) {
            auto exe = reinterpret_cast<ExecProgram *>(
                entries[i].lpCompletionKey
            );
            DWORD msg = entries[i].dwNumberOfBytesTransferred;
            DWORD processId = (DWORD) (uintptr_t) entries[i].lpOverlapped;
            (void) processId;
            if (msg == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO) {
                taskPush(exe->queue(), [=](){ exe->onProcessExit(); });
            }
        }
    }

    scoped_lock lk{s_mut};
    s_iocp = 0;
}

//===========================================================================
static HANDLE iocpHandle() {
    scoped_lock lk(s_mut);
    if (s_iocp)
        return s_iocp;

    s_iocp = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL, // existing port
        NULL, // completion key
        0     // concurrent threads, 0 for default
    );
    if (!s_iocp)
        logMsgFatal() << "CreateIoCompletionPort(null): " << WinError{};

    // Start IOCP dispatch task
    taskPushOnce("JobObject Dispatch", jobObjectIocpThread);
    return s_iocp;
}


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
// static
void ExecProgram::write(IExecNotify * notify, string_view data) {
    if (notify->m_exec)
        pipeWrite(&notify->m_exec->m_pipes[kStdIn], data);
}

//===========================================================================
ExecProgram::ExecProgram(IExecNotify * notify, TaskQueueHandle hq)
    : m_notify(notify)
    , m_hq(hq)
{
    s_perfPrograms += 1;
    s_perfCurPrograms += 1;

    for (auto && e : { kStdIn, kStdOut, kStdErr }) {
        m_pipes[e].m_strm = e;
        m_pipes[e].m_notify = this;
    }
}

//===========================================================================
ExecProgram::~ExecProgram() {
    assert(!m_notify);

    for (auto && pi : m_pipes)
        assert(pi.m_closed);

    s_perfCurPrograms -= 1;
}

//===========================================================================
void ExecProgram::exec(string_view cmdline, const ExecOptions & opts) {
    // Now that we're committed to getting an onProcessExit() call, change mode
    // so that we'll wait for it.
    m_mode = kRunStarting;

    bool success = false;
    for (;;) {
        m_job = CreateJobObject(NULL, NULL);
        if (!m_job) {
            logMsgError() << "CreateJobObjectW()" << WinError{};
            break;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION ei = {};
        auto & bi = ei.BasicLimitInformation;
        bi.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(
            m_job,
            JobObjectExtendedLimitInformation,
            &ei,
            sizeof ei
        )) {
            logMsgError() << "SetInformationJobObject(KILL_ON_JOB_CLOSE): "
                << WinError{};
            break;
        }
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT ap = {};
        ap.CompletionKey = this;
        ap.CompletionPort = iocpHandle();
        if (!SetInformationJobObject(
            m_job,
            JobObjectAssociateCompletionPortInformation,
            &ap,
            sizeof ap
        )) {
            logMsgError() << "SetInformationJobObject(ASSOC_IOCP): "
                << WinError{};
            break;
        }

        success = true;
        break;
    }
    if (!success) {
        onProcessExit();
        return;
    }

    char rawname[100];
    snprintf(
        rawname,
        sizeof rawname,
        "//./pipe/local/%u_%p",
        envProcessId(),
        this
    );
    string name = rawname;

    struct {
        StdStream strm;
        string name;
        Pipe::OpenMode oflags;
        HANDLE child;
    } pipes[] = {
        { kStdIn, name + ".in", Pipe::fReadWrite },
        { kStdOut, name + ".out", Pipe::fReadOnly },
        { kStdErr, name + ".err", Pipe::fReadOnly },
    };
    for (auto && p : pipes) {
        if (!createPipe(&p.child, p.strm, p.name, p.oflags)) {
            for (auto && p2 : pipes) {
                if (p2.child)
                    CloseHandle(p2.child);
            }
            onProcessExit();
            return;
        }
    }

    auto wcmdline = toWstring(cmdline);
    auto wworkDir = toWstring(opts.workingDir);
    STARTUPINFOW si = { sizeof si };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = pipes[kStdIn].child;
    si.hStdOutput = pipes[kStdOut].child;
    si.hStdError = pipes[kStdErr].child;
    PROCESS_INFORMATION pi = {};
    bool running = CreateProcessW(
        NULL, // explicit application name (no search path)
        wcmdline.data(),
        NULL, // process attrs
        NULL, // thread attrs
        true, // inherit handles, true to inherit the pipe handles
        CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED,
        NULL, // environment
        wworkDir.empty() ? NULL : wworkDir.c_str(), // current dir
        &si,
        &pi
    );
    WinError err;

    for (auto && p : pipes)
        CloseHandle(p.child);

    if (!running) {
        logMsgError() << "CreateProcessW(" << cmdline << "): " << err;
        onProcessExit();
        return;
    }

    m_process = pi.hProcess;
    if (!AssignProcessToJobObject(m_job, m_process)) {
        logMsgError() << "AssignProcessToJobObject: " << WinError{};
        onProcessExit();
        return;
    }

    // Now that the process has been assigned to the job it can be resumed.
    // If it had launched a process before joining the job that child would
    // be untracked.
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    m_mode = kRunRunning;
    if (!opts.stdinData.empty())
        execWrite(m_notify, opts.stdinData);
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
    unique_lock lk(m_mut);
    auto & pi = m_pipes[strm];
    pi.m_closed = true;
    if (completeIfDone_LK())
        lk.release();
}

//===========================================================================
void ExecProgram::onProcessExit() {
    unique_lock lk(m_mut);
    if (m_mode == kRunStarting) {
        for (auto && pi : m_pipes)
            pipeClose(&pi);
    }
    m_mode = kRunStopped;
    DWORD rc;
    if (GetExitCodeProcess(m_process, &rc)) {
        m_canceled = false;
        m_exitCode = rc;
    } else {
        m_canceled = true;
        m_exitCode = -1;
    }
    CloseHandle(m_process);
    CloseHandle(m_job);
    if (completeIfDone_LK())
        lk.release();
}

//===========================================================================
bool ExecProgram::completeIfDone_LK() {
    if (!m_pipes[kStdIn].m_closed
        || !m_pipes[kStdOut].m_closed
        || !m_pipes[kStdErr].m_closed
        || m_mode != kRunStopped
    ) {
        return false;
    }

    if (m_notify) {
        m_notify->m_exec = nullptr;
        m_notify->onExecComplete(m_canceled, m_exitCode);
        m_notify = nullptr;
    }

    m_mut.unlock();
    delete this;
    return true;
}

//===========================================================================
bool ExecProgram::createPipe(
    HANDLE * child,
    StdStream strm,
    string_view name,
    Pipe::OpenMode oflags
) {
    m_pipes[strm].m_closed = false;
    pipeListen(&m_pipes[strm], name, oflags, queue());

    if (child) {
        unsigned flags = SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION;
        // Child side has read & write reversed from listening parent
        if (oflags & Pipe::fReadOnly)
            flags |= GENERIC_WRITE;
        if (oflags & Pipe::fWriteOnly)
            flags |= GENERIC_READ;

        auto wname = toWstring(name);
        SECURITY_ATTRIBUTES sa = {};
        sa.bInheritHandle = true;
        auto cp = CreateFileW(
            wname.c_str(),
            flags,
            0,      // share
            &sa,
            OPEN_EXISTING,
            0,      // attributes and flags
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
    std::string_view cmdline,
    const ExecOptions & opts
) {
    assert(notify);

    char bytes[8];
    cryptRandomBytes(bytes, sizeof bytes);

    auto hq = taskInEventThread() ? taskComputeQueue() : taskEventQueue();
    auto ep = new ExecProgram(notify, hq);
    ep->exec(cmdline, opts);
}

//===========================================================================
void Dim::execProgram(
    IExecNotify * notify,
    const std::vector<std::string> & args,
    const ExecOptions & opts
) {
    execProgram(notify, Cli::toCmdline(args), opts);
}

//===========================================================================
void Dim::execWrite(IExecNotify * notify, std::string_view data) {
    ExecProgram::write(notify, data);
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

    bool wait(ExecResult * res);

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
    {
        scoped_lock lk{m_mut};
        m_complete = true;
        m_canceled = canceled;
        m_exitCode = exitCode;
    }
    m_cv.notify_one();
}

//===========================================================================
bool ExecWaitNotify::wait(ExecResult * res) {
    unique_lock lk{m_mut};
    while (!m_complete)
        m_cv.wait(lk);
    *res = {};
    res->out = move(m_out);
    res->err = move(m_err);
    res->exitCode = m_exitCode;
    return !m_canceled;
}

//===========================================================================
bool Dim::execProgramWait(
    ExecResult * res,
    std::string_view cmdline,
    const ExecOptions & opts
) {
    ExecWaitNotify notify;
    execProgram(&notify, cmdline, opts);
    return notify.wait(res);
}

//===========================================================================
bool Dim::execProgramWait(
    ExecResult * res,
    const std::vector<std::string> & args,
    const ExecOptions & opts
) {
    return execProgramWait(res, Cli::toCmdline(args), opts);
}


/****************************************************************************
*
*   Elevated
*
***/

//===========================================================================
bool Dim::execElevatedWait(
    int * exitCode,
    string_view cmdline,
    string_view workingDir
) {
    SHELLEXECUTEINFOW ei = { sizeof ei };
    ei.lpVerb = L"RunAs";
    auto args = Cli::toArgv(string(cmdline));
    auto wexe = toWstring(args.empty() ? "" : args[0]);
    ei.lpFile = wexe.c_str();
    auto wargs = toWstring(cmdline);
    ei.lpParameters = wargs.c_str();
    auto wdir = toWstring(workingDir);
    if (wdir.size())
        ei.lpDirectory = wdir.c_str();
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

    if (!ei.hProcess) {
        *exitCode = EX_OK;
    } else {
        DWORD rc = EX_OSERR;
        if (WAIT_OBJECT_0 == WaitForSingleObject(ei.hProcess, INFINITE))
            GetExitCodeProcess(ei.hProcess, &rc);
        *exitCode = rc;
        CloseHandle(ei.hProcess);
    }
    return true;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownServer(bool firstTry) override;
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownServer(bool firstTry) {
    if (s_perfCurPrograms)
        return shutdownIncomplete();
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    scoped_lock lk(s_mut);
    if (firstTry && s_iocp) {
        auto h = s_iocp;
        s_iocp = INVALID_HANDLE_VALUE;
        if (!CloseHandle(h))
            logMsgError() << "CloseHandle(IOCP): " << WinError{};
        Sleep(0);
    }

    if (s_iocp)
        shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::winExecInitialize() {
    shutdownMonitor(&s_cleanup);
}
