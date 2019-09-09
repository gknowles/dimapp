// Copyright Glen Knowles 2015 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// tnet.cpp - tnet
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

char const kVersion[] = "1.0";

enum { kExitConnectFailed = EX__APPBASE, kExitDisconnect };


class SocketConn
    : public ISocketNotify
    , public ISockAddrNotify
    , public ITimerNotify
{
    // Inherited via ISocketNotify
    void onSocketConnect(SocketInfo const & info) override;
    void onSocketConnectFailed() override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    bool onSocketRead(SocketData & data) override;
    void onSocketBufferChanged(SocketBufferInfo const & info) override;

    // Inherited via ISockAddrNotify
    void onSockAddrFound(SockAddr const * ptr, int count) override;

    // Inherited via ITimerNotify
    Duration onTimer(TimePoint now) override;

private:
    unique_ptr<ConsoleScopedAttr> m_connected;
};

class ConsoleReader : public IFileReadNotify {
public:
    void init();
    bool queryDestroy();
    void read(int64_t offset = 0);
    void suspend(bool suspend);

private:
    bool onFileRead(
        size_t * bytesUsed,
        string_view data,
        bool more,
        int64_t offset,
        FileHandle f
    ) override;

    FileHandle m_input;
    bool m_isFile{false};
    unique_ptr<SocketBuffer> m_buffer;

    bool m_suspended{false};
    int64_t m_offset{0};
};


/****************************************************************************
*
*   Variables
*
***/

static SockAddr s_localEnd;
static int s_cancelAddrId;
static ConsoleReader s_console;
static SocketConn s_socket;


/****************************************************************************
*
*   SocketConn
*
***/

//===========================================================================
void SocketConn::onSockAddrFound(SockAddr const * ends, int count) {
    if (!count) {
        cout << "Host not found" << endl;
        appSignalShutdown(kExitConnectFailed);
    } else {
        cout << "Connecting on " << s_localEnd << " to " << *ends << endl;
        socketConnect(this, *ends, s_localEnd);
    }
}

//===========================================================================
void SocketConn::onSocketConnect(SocketInfo const & info) {
    m_connected = make_unique<ConsoleScopedAttr>(kConsoleGreen);
    cout << "Connected" << endl;
    timerUpdate(this, 500ms);
}

//===========================================================================
void SocketConn::onSocketConnectFailed() {
    cout << "Connect failed" << endl;
    appSignalShutdown(kExitConnectFailed);
}

//===========================================================================
void SocketConn::onSocketDisconnect() {
    m_connected.reset();
    appSignalShutdown(kExitDisconnect);
}

//===========================================================================
void SocketConn::onSocketDestroy() {
    // it's a static, so override the default "delete this;" with a no-op
}

//===========================================================================
bool SocketConn::onSocketRead(SocketData & data) {
    cout.write(data.data, data.bytes);
    cout.flush();
    return true;
}

//===========================================================================
void SocketConn::onSocketBufferChanged(SocketBufferInfo const & info) {
    s_console.suspend(info.waiting);
}

//===========================================================================
Duration SocketConn::onTimer(TimePoint now) {
    s_console.read();
    return kTimerInfinite;
}


/****************************************************************************
*
*   ConsoleReader
*
***/

//===========================================================================
void ConsoleReader::init() {
    m_input = fileAttachStdin();
    if (!m_input)
        return appSignalShutdown(EX_IOERR);
    m_isFile = (fileType(m_input) == File::kRegular);
    if (!m_isFile)
        consoleEnableEcho(false);
}

//===========================================================================
bool ConsoleReader::queryDestroy() {
    if (m_input) {
        fileClose(m_input);
        m_input = {};
    }
    return !m_buffer;
}

//===========================================================================
void ConsoleReader::read(int64_t offset) {
    assert(m_input);
    m_buffer = socketGetBuffer();
    fileRead(this, m_buffer->data, m_buffer->capacity, m_input, offset);
}

//===========================================================================
void ConsoleReader::suspend(bool suspend) {
    assert(m_input);
    if (suspend == m_suspended)
        return;
    m_suspended = suspend;
    if (!suspend)
        read(m_offset);
}

//===========================================================================
bool ConsoleReader::onFileRead(
    size_t * bytesRead,
    string_view data,
    bool more,
    int64_t offset,
    FileHandle f
) {
    *bytesRead = data.size();
    auto bytes = (int) data.size();
    socketWrite(&s_socket, move(m_buffer), bytes);

    // stop reading (return false) so we can get a new buffer
    if (m_input) {
        if (m_isFile) {
            if (!bytes || (size_t) offset == fileSize(f)) {
                fileClose(m_input);
                consoleResetStdin();
                init();
            }
        } else {
            if (!bytes) {
                m_buffer.reset();
                return false;
            }
        }
        if (m_suspended) {
            m_offset = offset;
        } else {
            read(offset);
        }
    } else {
        m_buffer.reset();
    }
    return false;
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
    if (firstTry) {
        addressCancelQuery(s_cancelAddrId);
        socketDisconnect(&s_socket);
    }

    if (socketGetMode(&s_socket) != ISocketNotify::kInactive
        || !s_console.queryDestroy()
    ) {
        shutdownIncomplete();
    }
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    shutdownMonitor(&s_cleanup);

    Cli cli;
    cli.header("tnet v"s + kVersion + " (" __DATE__ ")")
        .helpNoArgs()
        .versionOpt(kVersion, "tnet");
    auto & remote = cli.opt<string>("<remote address>");
    cli.opt(&s_localEnd, "[local address]");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    consoleCatchCtrlC();
    s_console.init();

    addressQuery(&s_cancelAddrId, &s_socket, *remote, 23);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    return appRun(app, argc, argv);
}
