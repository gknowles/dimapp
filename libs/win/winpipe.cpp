// Copyright Glen Knowles 2018 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// winpipe.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const int kReadQueueSize = 1;
const int kWriteQueueSize = 100;

// How long data can wait to be sent. When the queue time exceeds this value
// the pipe is closed, the assumption being that the end consumer, if they
// still care, has a retry in flight and would discard it anyway as being
// expired.
constexpr auto kMaxPrewriteQueueTime = 2min;

const int kBufferSize = 4096;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

enum RequestType {
    kReqInvalid,
    kReqRead,
    kReqWrite,
};

struct PipeRequest : ListLink<>, IWinOverlappedNotify {
    RequestType m_type{kReqInvalid};
    string m_buffer;
    TimePoint m_qtime;

    // Inherited via IWinOverlappedNotify
    using IWinOverlappedNotify::IWinOverlappedNotify;
    void onTask() override;
};

} // namespace

class Dim::PipeBase {
public:
    using Mode = IPipeNotify::Mode;
    using OpenMode = Pipe::OpenMode;

public:
    static PipeBase::Mode getMode(IPipeNotify * notify);
    static void close(IPipeNotify * notify);
    static void write(IPipeNotify * notify, string_view data);
    static void read(IPipeNotify * notify);
    static void setNotify(shared_ptr<PipeBase> pipe, IPipeNotify * notify);
    static void setNotify(IPipeNotify * notify, IPipeNotify * newNotify);

public:
    PipeBase(string_view name, Pipe::OpenMode oflags, TaskQueueHandle hq);
    virtual ~PipeBase();

    void hardClose();
    void createQueue();
    void enableEvents(IPipeNotify * notify);

    // NOTE: If onRead or onWrite return false the pipe has been deleted.
    //       Additionally the task is completed and may be deleted whether or
    //       not false is returned.
    bool onRead(PipeRequest * task);
    bool onWrite(PipeRequest * task);

protected:
    mutex m_mut;
    IPipeNotify * m_notify{};
    string m_name;
    OpenMode m_oflags{};
    HANDLE m_handle{INVALID_HANDLE_VALUE};
    Mode m_mode{Mode::kInactive};
    shared_ptr<PipeBase> m_selfRef;

private:
    void requeueRead();
    void queueRead(PipeRequest * task);
    void queuePrewrite(string_view data);
    void queueWrites();
    bool onRead_LK(PipeRequest * task);

    PipeBufferInfo m_bufInfo{};
    TaskQueueHandle m_hq;

    // used by read requests
    List<PipeRequest> m_reads;
    int m_maxReads{0};
    List<PipeRequest> m_prereads;

    // used by write requests
    List<PipeRequest> m_writes;
    int m_numWrites{0};
    int m_maxWrites{0};
    List<PipeRequest> m_prewrites;
};


/****************************************************************************
*
*   Variables
*
***/

static atomic_int s_numPipes;

static auto & s_perfReadTotal = uperf("pipe.read bytes (total)");
static auto & s_perfIncomplete = uperf("pipe.write bytes (incomplete)");
static auto & s_perfWaiting = uperf("pipe.write bytes (waiting)");
static auto & s_perfWriteTotal = uperf("pipe.write bytes (total)");

// The data in the send queue is either too massive or too old.
static auto & s_perfBacklog = uperf("pipe.disconnect (write backlog)");


/****************************************************************************
*
*   PipeRequest
*
***/

//===========================================================================
void PipeRequest::onTask() {
    auto pipe = (PipeBase *) overlappedKey();

    // This task - and the pipe - may be deleted inside onRead/onWrite.
    if (m_type == kReqRead) {
        pipe->onRead(this);
    } else {
        assert(m_type == kReqWrite);
        pipe->onWrite(this);
    }
}


/****************************************************************************
*
*   PipeBase
*
***/

//===========================================================================
// static
PipeBase::Mode PipeBase::getMode(IPipeNotify * notify) {
    if (auto pipe = notify->m_pipe) {
        scoped_lock lk{pipe->m_mut};
        return pipe->m_mode;
    }

    return Mode::kInactive;
}

//===========================================================================
// static
void PipeBase::close(IPipeNotify * notify) {
    if (auto pipe = notify->m_pipe) {
        scoped_lock lk{pipe->m_mut};
        pipe->hardClose();
    }
}

//===========================================================================
// static
void PipeBase::setNotify(IPipeNotify * notify, IPipeNotify * newNotify) {
    if (auto pipe = notify->m_pipe) {
        scoped_lock lk{pipe->m_mut};
        notify->m_pipe.reset();
        pipe->m_notify = newNotify;
        newNotify->m_pipe = pipe;
    }
}

//===========================================================================
// static
void PipeBase::setNotify(shared_ptr<PipeBase> pipe, IPipeNotify * notify) {
    scoped_lock lk{pipe->m_mut};
    assert(!notify->m_pipe && !pipe->m_notify);
    notify->m_pipe = pipe;
    pipe->m_selfRef = pipe;
    pipe->m_notify = notify;
}

//===========================================================================
PipeBase::PipeBase(
    string_view name,
    Pipe::OpenMode oflags,
    TaskQueueHandle hq
)
    : m_name(name)
    , m_oflags(oflags)
    , m_hq(hq)
{
    s_numPipes += 1;
}

//===========================================================================
PipeBase::~PipeBase() {
    assert(!m_reads && !m_writes);

    if (m_notify)
        m_notify->m_pipe = nullptr;

    hardClose();
    m_prereads.clear();

    s_numPipes -= 1;
}

//===========================================================================
void PipeBase::hardClose() {
    if (m_handle == INVALID_HANDLE_VALUE)
        return;

    CloseHandle(m_handle);

    m_mode = Mode::kClosing;
    m_handle = INVALID_HANDLE_VALUE;
    while (auto req = m_prewrites.front()) {
        auto bytes = (unsigned) req->m_buffer.size();
        m_bufInfo.waiting -= bytes;
        s_perfWaiting -= bytes;
        delete req;
    }
}

//===========================================================================
void PipeBase::createQueue() {
    if (m_oflags & Pipe::fReadOnly) {
        m_maxReads = kReadQueueSize;
        m_maxWrites = 0;
    } else if (m_oflags & Pipe::fWriteOnly) {
        m_maxReads = 0;
        m_maxWrites = kWriteQueueSize;
    } else {
        assert(m_oflags & Pipe::fReadWrite);
        m_maxReads = kReadQueueSize;
        m_maxWrites = kWriteQueueSize;
    }

    for (int i = 0; i < m_maxReads; ++i) {
        m_prereads.link(new PipeRequest(m_hq));
        auto task = m_prereads.back();
        task->m_type = kReqRead;
        task->m_buffer.resize(kBufferSize);
    }

    m_mode = Mode::kActive;
}

//===========================================================================
void PipeBase::enableEvents(IPipeNotify * notify) {
    if (notify)
        m_notify = notify;

    // trigger first reads
    while (m_prereads)
        requeueRead();
}

//===========================================================================
// static
void PipeBase::read(IPipeNotify * notify) {
    if (auto pipe = notify->m_pipe) {
        scoped_lock lk{pipe->m_mut};
        pipe->requeueRead();
    }
}

//===========================================================================
void PipeBase::requeueRead() {
    auto task = m_prereads.front();

    // Queuing reads is only allowed after an automatic requeuing was
    // rejected via returning false from onSocketRead.
    assert(task);

    if (m_mode == Mode::kActive) {
        m_reads.link(task);
        queueRead(task);
    } else {
        assert(m_mode == Mode::kClosing || m_mode == Mode::kClosed);
        task->overlapped() = {};
        onRead_LK(task);
    }
}

//===========================================================================
void PipeBase::queueRead(PipeRequest * task) {
    task->m_buffer.resize(kBufferSize);
    task->overlapped() = {};
    WinError err{0};
    if (!ReadFile(
        m_handle,
        task->m_buffer.data(),
        (DWORD) task->m_buffer.size(),
        nullptr,
        &task->overlapped()
    )) {
        err.set();
    }
    if (err == ERROR_IO_PENDING) {
        // expected, since it's attached to a completion port
    } else if (err == ERROR_BROKEN_PIPE) {
        task->pushOverlappedTask();
    } else {
        logMsgFatal() << "ReadFile(pipe): " << err;
    }
}

//===========================================================================
bool PipeBase::onRead(PipeRequest * task) {
    unique_lock lk(m_mut);
    return onRead_LK(task);
}

//===========================================================================
bool PipeBase::onRead_LK(PipeRequest * task) {
    auto res = task->decodeOverlappedResult();
    if (int bytes = res.bytes) {
        s_perfReadTotal += bytes;
        task->m_buffer.resize(bytes);
        size_t used{0};
        m_mut.unlock();
        bool more = m_notify->onPipeRead(&used, task->m_buffer);
        m_mut.lock();
        assert(used == bytes);
        if (!more) {
            // new read will be explicitly requested by application
            m_prereads.link(task);
            return true;
        }

        if (m_mode == Mode::kActive) {
            queueRead(task);
            return true;
        }

        assert(m_mode == Mode::kClosing || m_mode == Mode::kClosed);
    }

    delete task;

    if (m_mode != Mode::kClosed) {
        m_mode = Mode::kClosed;
        m_mut.unlock();
        m_notify->onPipeDisconnect();
        m_mut.lock();
    }

    if (!m_reads && !m_writes) {
        shared_ptr<PipeBase> ptr = std::move(m_selfRef);
        return false;
    }
    return true;
}

//===========================================================================
// static
void PipeBase::write(IPipeNotify * notify, string_view data) {
    if (!data.size())
        return;
    if (auto pipe = notify->m_pipe) {
        scoped_lock lk{pipe->m_mut};
        pipe->queuePrewrite(data);
    }
}

//===========================================================================
void PipeBase::queuePrewrite(string_view data) {
    assert(data.size());
    if (m_mode == Mode::kClosing || m_mode == Mode::kClosed)
        return;

    auto bytes = data.size();
    s_perfWaiting += (unsigned) bytes;
    s_perfWriteTotal += (unsigned) bytes;
    m_bufInfo.waiting += bytes;
    m_bufInfo.total += bytes;
    bool wasPrewrites = (bool) m_prewrites;

    if (wasPrewrites) {
        auto back = m_prewrites.back();
        if (auto count = (int) min(
            back->m_buffer.capacity() - back->m_buffer.size(),
            bytes
        )) {
            back->m_buffer.append(data.data(), count);
            data.remove_prefix(count);
            bytes -= count;
        }
    }

    if (bytes) {
        m_prewrites.link(new PipeRequest(m_hq));
        auto task = m_prewrites.back();
        task->m_type = kReqWrite;
        task->m_buffer.reserve(max(data.size(), (size_t) kBufferSize));
        task->m_buffer = data;
    }

    queueWrites();
    if (auto task = m_prewrites.back()) {
        if (task->m_qtime == TimePoint::min()) {
            auto now = timeNow();
            task->m_qtime = now;
            auto maxQTime = now - m_prewrites.front()->m_qtime;
            if (maxQTime > kMaxPrewriteQueueTime) {
                s_perfBacklog += 1;
                return hardClose();
            }
        }
        if (!wasPrewrites) {
            // data is now waiting
            assert(m_bufInfo.waiting <= bytes);
            auto info = m_bufInfo;
            m_notify->onPipeBufferChanged(info);
        }
    }
}

//===========================================================================
void PipeBase::queueWrites() {
    while (m_numWrites < m_maxWrites && m_prewrites) {
        m_writes.link(m_prewrites.front());
        m_numWrites += 1;
        auto task = m_writes.back();
        auto bytes = (unsigned) task->m_buffer.size();
        s_perfWaiting -= bytes;
        m_bufInfo.waiting -= bytes;
        WinError err{0};
        if (!WriteFile(
            m_handle,
            task->m_buffer.data(),
            bytes,
            nullptr,
            &task->overlapped()
        )) {
            err.set();
        }
        if (err != ERROR_IO_PENDING) {
            logMsgFatal() << "WriteFile(pipe): " << err;
            delete task;
            m_numWrites -= 1;
        } else {
            s_perfIncomplete += bytes;
            m_bufInfo.incomplete += bytes;
        }
    }
}

//===========================================================================
bool PipeBase::onWrite(PipeRequest * task) {
    scoped_lock lk{m_mut};

    auto bytes = task->decodeOverlappedResult().bytes;
    delete task;
    m_numWrites -= 1;
    s_perfIncomplete -= bytes;
    m_bufInfo.incomplete -= bytes;

    // already disconnected and this was the last unresolved write? delete
    if (m_mode == Mode::kClosed && !m_reads && !m_writes) {
        delete this;
        return false;
    }

    bool wasPrewrites = (bool) m_prewrites;
    queueWrites();
    if (wasPrewrites && !m_prewrites // data no longer waiting
        || !m_numWrites              // send queue is empty
    ) {
        assert(!m_bufInfo.waiting);
        assert(m_numWrites || !m_bufInfo.incomplete);
        auto info = m_bufInfo;
        m_notify->onPipeBufferChanged(info);
    }
    return true;
}


/****************************************************************************
*
*   Listen
*
***/

namespace {

class ListenPipe : public ListLink<>, public IPipeNotify {
public:
    IFactory<IPipeNotify> * m_factory = {};
    string m_name;
    Pipe::OpenMode m_oflags = {};
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    RunMode m_mode = kRunStopped;

    void start();
    void stop();

    // Inherited via IPipeNotify
    bool onPipeAccept() override;
    bool onPipeRead(size_t * bytesUsed, string_view data) override;
    void onPipeDisconnect() override;
};

class AcceptPipe : public PipeBase, public IWinOverlappedNotify {
public:
    static void listen(
        IPipeNotify * notify,
        string_view pipeName,
        Pipe::OpenMode oflags,
        TaskQueueHandle hq
    );

public:
    AcceptPipe(string_view name, Pipe::OpenMode oflags, TaskQueueHandle hq);
    ~AcceptPipe();
    void connect();

    void onTask() override;
};

} // namespace

static List<ListenPipe> s_listeners;
static mutex s_listenersMut;

static auto & s_perfAccepts = uperf("pipe.accepts");
static auto & s_perfCurAccepts = uperf("pipe.accepts (current)");
static auto & s_perfNotAccepted = uperf("pipe.disconnect (not accepted)");


//===========================================================================
// AcceptPipe
//===========================================================================
AcceptPipe::AcceptPipe(
    string_view name,
    Pipe::OpenMode oflags,
    TaskQueueHandle hq
)
    : PipeBase(name, oflags, hq)
    , IWinOverlappedNotify(hq)
{}

//===========================================================================
AcceptPipe::~AcceptPipe() {
    if (m_mode == Mode::kClosed)
        s_perfCurAccepts -= 1;
}

//===========================================================================
// static
void AcceptPipe::listen(
    IPipeNotify * notify,
    string_view pipeName,
    Pipe::OpenMode oflags,
    TaskQueueHandle hq
) {
    auto pipe = make_shared<AcceptPipe>(pipeName, oflags, hq);
    PipeBase::setNotify(pipe, notify);
    pipe->connect();
}

//===========================================================================
void AcceptPipe::connect() {
    unique_lock lk{m_mut};

    auto wname = toWstring(m_name);
    DWORD flags = FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED;
    constexpr auto aflags =
        Pipe::fReadOnly | Pipe::fWriteOnly | Pipe::fReadWrite;
    switch (m_oflags & aflags) {
    default:
        assert(!"Invalid pipe open flags");
        break;
    case Pipe::fReadOnly:
        flags |= PIPE_ACCESS_INBOUND;
        break;
    case Pipe::fWriteOnly:
        flags |= PIPE_ACCESS_OUTBOUND;
        break;
    case Pipe::fReadWrite:
        flags |= PIPE_ACCESS_DUPLEX;
        break;
    }
    m_handle = CreateNamedPipeW(
        wname.c_str(),
        flags,
        PIPE_REJECT_REMOTE_CLIENTS,
        1,  // max instances
        0,  // out buffer size
        0,  // in buffer size,
        INFINITE,   // default timeout
        NULL        // security attributes
    );
    if (m_handle == INVALID_HANDLE_VALUE) {
        WinError err;
        lk.unlock();
        logMsgError() << "CreateNamedPipe: " << err;
        m_notify->onPipeDisconnect();
        delete this;
        return;
    }
    if (!winIocpBindHandle(m_handle, this)) {
        lk.unlock();
        m_notify->onPipeDisconnect();
        delete this;
        return;
    }
    if (ConnectNamedPipe(m_handle, &overlapped())) {
        logMsgFatal() << "ConnectNamedPipe: non-overlapped result";
        return;
    }
    WinError err;
    if (err == ERROR_PIPE_CONNECTED) {
        pushOverlappedTask();
    } else if (err != ERROR_IO_PENDING) {
        lk.unlock();
        logMsgError() << "ConnectNamedPipe: " << err;
        m_notify->onPipeDisconnect();
        delete this;
        return;
    }
    m_mode = Mode::kAccepting;
}

//===========================================================================
void AcceptPipe::onTask() {
    unique_lock lk{m_mut};

    auto [err, bytes] = decodeOverlappedResult();
    bool ok = m_handle != INVALID_HANDLE_VALUE;
    if (err) {
        if (!ok && err == ERROR_BROKEN_PIPE) {
            // intentionally closed
        } else {
            logMsgError() << "on pipe connect: " << err;
            ok = false;
        }
    }
    if (!ok) {
        lk.unlock();
        m_notify->onPipeDisconnect();
        delete this;
        return;
    }

    createQueue();
    s_perfAccepts += 1;
    s_perfCurAccepts += 1;
    lk.unlock();
    if (!m_notify->onPipeAccept()) {
        s_perfNotAccepted += 1;
        lk.lock();
        hardClose();
    } else {
        lk.lock();
    }
    enableEvents(nullptr);
}

//===========================================================================
void Dim::pipeListen(
    IPipeNotify * notify,
    string_view pipeName,
    Pipe::OpenMode oflags,
    TaskQueueHandle hq
) {
    AcceptPipe::listen(notify, pipeName, oflags, hq);
}


//===========================================================================
// ListenPipe
//===========================================================================
void ListenPipe::start() {
//    [[maybe_unused]] auto pipe = new AcceptPipe(this, m_name, m_oflags);
}

//===========================================================================
void ListenPipe::stop() {
    m_mode = kRunStopping;
    PipeBase::close(this);
}

//===========================================================================
bool ListenPipe::onPipeAccept() {
    auto notify = m_factory->onFactoryCreate().release();
    PipeBase::setNotify(this, notify);
    if (m_mode == kRunRunning)
        start();
    return notify->onPipeAccept();
}

//===========================================================================
bool ListenPipe::onPipeRead(size_t * bytesUsed, string_view data) {
    assert(!"Should never be called.");
    return false;
}

//===========================================================================
void ListenPipe::onPipeDisconnect() {
    if (m_mode == kRunStopping) {
        delete this;
    } else {
        assert(m_mode == kRunRunning);
        start();
    }
}

//===========================================================================
void Dim::pipeListen(
    IFactory<IPipeNotify> * factory,
    string_view name,
    Pipe::OpenMode oflags
) {
    auto listener = new ListenPipe;
    listener->m_mode = kRunRunning;
    listener->m_factory = factory;
    listener->m_name = name;
    listener->m_oflags = oflags;
    {
        scoped_lock lk{s_listenersMut};
        s_listeners.link(listener);
    }
    listener->start();
}

//===========================================================================
void Dim::pipeClose(
    IFactory<IPipeNotify> * factory,
    string_view pipeName
) {
    ListenPipe * found = nullptr;
    {
        scoped_lock lk{s_listenersMut};
        for (auto && ls : s_listeners) {
            if (ls.m_factory == factory && ls.m_name == pipeName) {
                found = &ls;
                break;
            }
        }
    }

    if (found)
        found->stop();
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (s_numPipes)
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iPipeInitialize() {
    shutdownMonitor(&s_cleanup);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
IPipeNotify::Mode Dim::pipeGetMode(IPipeNotify * notify) {
    return PipeBase::getMode(notify);
}

//===========================================================================
void Dim::pipeClose(IPipeNotify * notify) {
    PipeBase::close(notify);
}

//===========================================================================
void Dim::pipeWrite(IPipeNotify * notify, string_view data) {
    PipeBase::write(notify, data);
}

//===========================================================================
void Dim::pipeRead(IPipeNotify * notify) {
    PipeBase::read(notify);
}
