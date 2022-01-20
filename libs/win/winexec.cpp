// Copyright Glen Knowles 2019 - 2022.
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

class ExecProgram
    : public ListLink<>
    , public ITimerNotify
{
public:
    class ExecPipe : public IPipeNotify {
    public:
        // Inherited via IPipeNotify
        bool onPipeAccept() override;
        bool onPipeRead(size_t * bytesUsed, string_view data) override;
        void onPipeDisconnect() override;
        void onPipeBufferChanged(const PipeBufferInfo & info) override;

        ExecProgram * m_notify = {};
        StdStream m_strm = {};
        RunMode m_mode = kRunStopped;
        HANDLE m_child = {};
    };

public:
    static void write(IExecNotify * notify, std::string_view data);
    static void dequeue();

public:
    ExecProgram(
        IExecNotify * notify,
        string_view cmdline,
        const ExecOptions & opts
    );
    ~ExecProgram();
    void exec();
    TaskQueueHandle queue() const { return m_hq; }
    void terminate();
    void postJobExit();

    bool onAccept(StdStream strm);
    bool onRead(size_t * bytesUsed, StdStream strm, string_view data);
    void onDisconnect(StdStream strm);
    void onBufferChanged(StdStream strm, const PipeBufferInfo & info);

    void onJobExit();

    // Inherited via ITimerNotify
    Duration onTimer(TimePoint now) override;

private:
    bool createPipe(
        HANDLE * hchild,
        StdStream strm,
        string_view name,
        EnumFlags<Pipe::OpenMode> oflags
    );
    void completeIfDone_UNLK(unique_lock<mutex> && lk);

    TaskQueueHandle m_hq;
    HANDLE m_job = NULL;
    HANDLE m_process = NULL;
    HANDLE m_thread = NULL;
    string m_cmdline;
    ExecOptions m_opts;

    //-----------------------------------------------------------------------
    mutex m_mut;

    IExecNotify * m_notify{};
    ExecPipe m_pipes[3];
    unsigned m_connected{};

    bool m_exitPosted = false;
    RunMode m_mode{kRunStopped};
    ExecResult::Type m_exitType = ExecResult::kNotStarted;
    int m_exitCode = -1;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static auto & s_perfTotal = uperf("exec.programs");
static auto & s_perfIncomplete = uperf("exec.programs (incomplete)");
static auto & s_perfWaiting = uperf("exec.programs (waiting)");

static mutex s_mut;
static HANDLE s_iocp;
static List<ExecProgram> s_programs;


/****************************************************************************
*
*   IOCP completion thread
*
***/

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
            [[maybe_unused]] DWORD procId =
                (DWORD) (uintptr_t) entries[i].lpOverlapped;
            if (msg == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO) {
                // The process, and any child processes it may have launched,
                // have exited.
                taskPush(exe->queue(), [=](){ exe->onJobExit(); });
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
bool ExecProgram::ExecPipe::onPipeAccept() {
    return m_notify->onAccept(m_strm);
}

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

//===========================================================================
void ExecProgram::ExecPipe::onPipeBufferChanged(const PipeBufferInfo & info) {
    m_notify->onBufferChanged(m_strm, info);
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
// static
void ExecProgram::dequeue() {
    List<ExecProgram> progs;
    {
        scoped_lock lk{s_mut};
        while (auto prog = s_programs.front()) {
            if (prog->m_opts.concurrency <= s_perfIncomplete)
                break;
            s_perfWaiting -= 1;
            s_perfIncomplete += 1;
            progs.link(prog);
        }
    }
    while (auto prog = progs.front()) {
        prog->unlink();
        prog->exec();
    }
}

//===========================================================================
ExecProgram::ExecProgram(
    IExecNotify * notify,
    string_view cmdline,
    const ExecOptions & opts
)
    : m_cmdline(cmdline)
    , m_opts(opts)
    , m_notify(notify)
{
    s_perfTotal += 1;
    s_perfWaiting += 1;

    m_notify->m_exec = this;
    if (m_opts.concurrency == 0)
        m_opts.concurrency = envProcessors();

    for (auto && e : { kStdIn, kStdOut, kStdErr }) {
        m_pipes[e].m_strm = e;
        m_pipes[e].m_notify = this;
    }

    scoped_lock lk{s_mut};
    s_programs.link(this);
}

//===========================================================================
ExecProgram::~ExecProgram() {
    assert(!m_notify);

    for ([[maybe_unused]] auto && pi : m_pipes)
        assert(pi.m_mode == kRunStopped);

    if (linked()) {
        s_perfWaiting -= 1;
    } else {
        s_perfIncomplete -= 1;
    }
}

//===========================================================================
void ExecProgram::terminate() {
    unique_lock lk(m_mut);
    if (m_mode == kRunStopped || !m_process)
        return;

    if (!TerminateProcess(m_process, (UINT) -1)) {
        logMsgError() << "TerminateProcess(" << m_cmdline << "): "
            << WinError{};
    }
    CloseHandle(m_process);

    // Set to null handle so the process will be recognized as canceled.
    m_process = {};
}

//===========================================================================
void ExecProgram::postJobExit() {
    {
        unique_lock lk{m_mut};
        m_exitPosted = true;
    }

    // Exit events are posted to the job queue so the onExecComplete event gets
    // routed to the task queue selected by the application.
    if (!PostQueuedCompletionStatus(
        s_iocp,
        JOB_OBJECT_MSG_EXIT_PROCESS,
        (uintptr_t) this,
        (OVERLAPPED *) (uintptr_t) GetProcessId(m_process)
    )) {
        logMsgFatal() << "PostQueuedCompletionStatus: " << WinError{};
    }
}

//===========================================================================
void ExecProgram::exec() {
    // Now that we're committed to getting an onJobExit() call, change mode
    // so that we'll wait for it.
    m_mode = kRunStarting;
    m_hq = m_opts.hq;

    // Create pipes
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
        EnumFlags<Pipe::OpenMode> oflags;
    } pipes[] = {
        { kStdIn, name + ".in", Pipe::fReadWrite },
        { kStdOut, name + ".out", Pipe::fReadOnly },
        { kStdErr, name + ".err", Pipe::fReadOnly },
    };

    // First set all pipes to kRunStarting (not kRunStopped), so that the
    // following transition to stopped can be unambiguously detected.
    for (auto&& p : pipes)
        m_pipes[p.strm].m_mode = kRunStarting;

    for (auto && p : pipes) {
        auto & child = m_pipes[p.strm].m_child;
        if (!createPipe(&child, p.strm, p.name, p.oflags)) {
            postJobExit();
            return;
        }
    }
}

//===========================================================================
bool ExecProgram::onAccept(StdStream strm) {
    unique_lock lk(m_mut);
    assert(m_mode == kRunStarting);
    assert(m_pipes[strm].m_mode == kRunStarting);
    m_pipes[strm].m_mode = kRunRunning;
    if (m_pipes[kStdIn].m_mode == kRunStarting
        || m_pipes[kStdOut].m_mode == kRunStarting
        || m_pipes[kStdErr].m_mode == kRunStarting
    ) {
        // If it wasn't the last pipe to finish starting, continue waiting for
        // the last one.
        return true;
    }
    // Now that all pipes have started, check to make sure all are running, and
    // stop the whole exec if they aren't.
    if (m_pipes[kStdIn].m_mode != kRunRunning
        || m_pipes[kStdOut].m_mode != kRunRunning
        || m_pipes[kStdErr].m_mode != kRunRunning
    ) {
        lk.unlock();
        postJobExit();
        return true;
    }

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
        if (m_opts.untrackedChildren)
            bi.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
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
        lk.unlock();
        postJobExit();
        return true;
    }

    char * envBlk = nullptr;
    string envBuf;
    if (!m_opts.envVars.empty()) {
        auto env = envGetVars();
        for (auto&& [name, value] : m_opts.envVars) {
            if (value.empty()) {
                env.erase(name);
            } else {
                env[name] = value;
            }
        }
        for (auto&& [name, value] : env) {
            envBuf += name;
            envBuf += '=';
            envBuf += value;
            envBuf += '\0';
        }
        envBuf += '\0';
        envBlk = envBuf.data();
    }
    auto wcmdline = toWstring(m_cmdline);
    auto wworkDir = toWstring(m_opts.workingDir);
    STARTUPINFOW si = { sizeof si };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = m_pipes[kStdIn].m_child;
    si.hStdOutput = m_pipes[kStdOut].m_child;
    si.hStdError = m_pipes[kStdErr].m_child;
    PROCESS_INFORMATION pi = {};
    bool running = CreateProcessW(
        NULL, // explicit application name (no search path)
        wcmdline.data(),
        NULL, // process attrs
        NULL, // thread attrs
        true, // inherit handles, true to inherit the pipe handles
        CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED,
        envBlk,
        wworkDir.empty() ? NULL : wworkDir.c_str(), // current dir
        &si,
        &pi
    );
    WinError err;

    // Close parents reference to child side of pipes.
    for (auto && pipe : m_pipes) {
        if (pipe.m_child) {
            CloseHandle(pipe.m_child);
            pipe.m_child = {};
        }
    }

    if (!running) {
        logMsgError() << "CreateProcessW(" << m_cmdline << "): " << err;
        lk.unlock();
        postJobExit();
        return true;
    }

    m_process = pi.hProcess;
    m_thread = pi.hThread;
    if (!AssignProcessToJobObject(m_job, m_process)) {
        logMsgError() << "AssignProcessToJobObject(" << m_cmdline << "): "
            << WinError{};
        lk.unlock();
        terminate();
        postJobExit();
        return true;
    }

    // Now that the process has been assigned to the job it can be resumed.
    // If it had launched a process before joining the job that child would
    // be untracked.
    m_mode = kRunRunning;
    ResumeThread(m_thread);
    CloseHandle(m_thread);

    // Start the allowed execution time countdown.
    timerUpdate(this, m_opts.timeout);

    auto pipe = &m_pipes[kStdIn];
    if (!m_opts.stdinData.empty()) {
        // if !enableExecWrite the pipe will be closed when this write
        // completes.
        pipeWrite(pipe, m_opts.stdinData);
    } else if (!m_opts.enableExecWrite) {
        pipeClose(pipe);
    }
    return true;
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
    pi.m_mode = kRunStopped;
    completeIfDone_UNLK(move(lk));
}

//===========================================================================
void ExecProgram::onBufferChanged(
    StdStream strm,
    const PipeBufferInfo & info
) {
    if (strm == kStdIn && !m_opts.enableExecWrite && !info.incomplete)
        pipeClose(&m_pipes[strm]);
}

//===========================================================================
Duration ExecProgram::onTimer(TimePoint now) {
    terminate();
    return kTimerInfinite;
}

//===========================================================================
void ExecProgram::onJobExit() {
    assert(m_mode != kRunStopped);
    unique_lock lk(m_mut);
    for (auto && pi : m_pipes) {
        pipeClose(&pi);
        if (pi.m_child) {
            CloseHandle(pi.m_child);
            pi.m_child = {};
        }
    }
    auto modeWas = m_mode;
    m_mode = kRunStopped;
    DWORD rc;
    if (GetExitCodeProcess(m_process, &rc)) {
        m_exitType = ExecResult::kExited;
        m_exitCode = rc;
    } else {
        WinError err;
        m_exitType = modeWas == kRunStarting 
            ? ExecResult::kNotStarted
            : ExecResult::kCanceled;
        m_exitCode = -1;
    }
    CloseHandle(m_process);
    CloseHandle(m_job);
    completeIfDone_UNLK(move(lk));
}

//===========================================================================
void ExecProgram::completeIfDone_UNLK(unique_lock<mutex> && lk) {
    assert(lk);
    if (m_pipes[kStdIn].m_mode != kRunStopped
        || m_pipes[kStdOut].m_mode != kRunStopped
        || m_pipes[kStdErr].m_mode != kRunStopped
        || m_mode != kRunStopped
    ) {
        return;
    }

    if (m_notify) {
        m_notify->m_exec = nullptr;
        m_notify->onExecComplete(m_exitType, m_exitCode);
        m_notify = nullptr;
    }

    lk.unlock();
    lk.release();
    delete this;
    dequeue();
}

//===========================================================================
bool ExecProgram::createPipe(
    HANDLE * child,
    StdStream strm,
    string_view name,
    EnumFlags<Pipe::OpenMode> oflags
) {
    using enum Pipe::OpenMode;
    assert(m_pipes[strm].m_mode == kRunStarting);

    pipeListen(&m_pipes[strm], name, oflags, queue());
    if (child) {
        unsigned access = 0;
        unsigned flags = SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION;
        // Child side has read & write reversed from listening parent
        if (oflags.any(fReadOnly)) {
            access |= GENERIC_WRITE;
        } else if (oflags.any(fWriteOnly)) {
            access |= GENERIC_READ;
        } else if (oflags.any(fReadWrite)) {
            access |= GENERIC_READ | GENERIC_WRITE;
        }

        auto wname = toWstring(name);
        SECURITY_ATTRIBUTES sa = {};
        sa.bInheritHandle = true;
        auto cp = CreateFileW(
            wname.c_str(),
            access,
            0,      // share
            &sa,
            OPEN_EXISTING,
            flags,  // attributes and flags
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
    const std::string & cmdline,
    const ExecOptions & rawOpts
) {
    assert(notify);

    auto opts = rawOpts;
    if (!opts.hq)
        opts.hq = taskEventQueue();

    new ExecProgram(notify, cmdline, opts);
    ExecProgram::dequeue();
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
*   Simple execute child program
*
***/

namespace {

struct SimpleExecNotify : public IExecNotify {
    function<void(ExecResult && res)> m_fn;
    ExecResult m_res;

    void onExecComplete(ExecResult::Type exitType, int exitCode) override;
};

} // namespace

//===========================================================================
void SimpleExecNotify::onExecComplete(
    ExecResult::Type exitType, 
    int exitCode
) {
    m_res.exitType = exitType;
    m_res.exitCode = exitCode;
    m_res.out = move(m_out);
    m_res.err = move(m_err);
    m_fn(move(m_res));
    delete this;
}

//===========================================================================
void Dim::execProgram(
    function<void(ExecResult && res)> fn,
    const string & cmdline,
    const ExecOptions & opts
) {
    auto notify = new SimpleExecNotify;
    notify->m_fn = fn;
    notify->m_res.cmdline = cmdline;
    execProgram(notify, cmdline, opts);
}

//===========================================================================
void Dim::execProgram(
    function<void(ExecResult && res)> fn,
    const vector<string> & args,
    const ExecOptions & opts
) {
    execProgram(fn, Cli::toCmdline(args), opts);
}


/****************************************************************************
*
*   Execute child program and wait
*
***/

//===========================================================================
bool Dim::execProgramWait(
    ExecResult * out,
    const string & cmdline,
    const ExecOptions & rawOpts
) {
    auto opts = rawOpts;
    if (!opts.hq)
        opts.hq = taskInEventThread() ? taskComputeQueue() : taskEventQueue();

    latch lat(1);
    out->cmdline = cmdline;
    execProgram(
        [&](ExecResult && res) {
            *out = move(res);
            lat.count_down();
        },
        cmdline,
        opts
    );
    lat.wait();
    return out->exitCode != -1;
}

//===========================================================================
bool Dim::execProgramWait(
    ExecResult * res,
    const vector<string> & args,
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
    const string & cmdline,
    const string & workingDir
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
*   External
*
***/

//===========================================================================
void Dim::execCancelWaiting() {
    List<ExecProgram> progs;
    {
        scoped_lock lk{s_mut};
        progs.link(move(s_programs));
    }
    while (auto prog = progs.front()) {
        s_programs.unlink(prog);
        prog->postJobExit();
    }
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
    if (s_perfIncomplete || s_perfWaiting)
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
