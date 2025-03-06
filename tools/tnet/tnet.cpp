// Copyright Glen Knowles 2015 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// tnet.cpp - tnet
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 1, 0 };


/****************************************************************************
*
*   Declarations
*
***/

enum { kExitConnectFailed = EX__APPBASE, kExitDisconnect };


class SocketConn
    : public ISocketNotify
    , public ISockAddrNotify
    , public ITimerNotify
{
    // Inherited via ISocketNotify
    void onSocketConnect(const SocketConnectInfo & info) override;
    void onSocketConnectFailed() override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    bool onSocketRead(SocketData & data) override;
    void onSocketBufferChanged(const SocketBufferInfo & info) override;

    // Inherited via ISockAddrNotify
    void onSockAddrFound(const SockAddr * ptr, int count) override;

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
    bool onFileRead(size_t * bytesUsed, const FileReadData & data) override;

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

static SockAddr s_localAddr;
static string s_remoteAddrStr;
static int s_cancelAddrId;
static ConsoleReader s_console;
static SocketConn s_socket;


/****************************************************************************
*
*   SocketConn
*
***/

//===========================================================================
void SocketConn::onSockAddrFound(const SockAddr * ends, int count) {
    if (!count) {
        cout << "Host not found" << endl;
        appSignalShutdown(kExitConnectFailed);
    } else {
        cout << "Connecting on " << s_localAddr << " to " << *ends << endl;
        socketConnect(this, *ends, s_localAddr);
    }
}

//===========================================================================
void SocketConn::onSocketConnect(const SocketConnectInfo & info) {
    m_connected = make_unique<ConsoleScopedAttr>(kConsoleCheer);
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
void SocketConn::onSocketBufferChanged(const SocketBufferInfo & info) {
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
    if (auto ec = fileAttachStdin(&m_input); ec)
        return appSignalShutdown(EX_IOERR);
    File::Type ftype = File::Type::kUnknown;
    fileType(&ftype, m_input);
    m_isFile = (ftype == File::Type::kRegular);
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
    const FileReadData & data
) {
    *bytesRead = data.data.size();
    auto bytes = (int) data.data.size();
    socketWrite(&s_socket, move(m_buffer), bytes);

    // stop reading (return false) so we can get a new buffer
    if (m_input) {
        if (m_isFile) {
            uint64_t fsize = 0;
            if (!bytes
                || fileSize(&fsize, data.f)
                || (uint64_t) data.offset == fsize
            ) {
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
            m_offset = data.offset;
        } else {
            read(data.offset);
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
static void app(Cli & cli) {
    shutdownMonitor(&s_cleanup);
    consoleCatchCtrlC();
    s_console.init();
    addressQuery(&s_cancelAddrId, &s_socket, s_remoteAddrStr, 23);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    Cli cli;
    cli.helpNoArgs();
    cli.opt(&s_remoteAddrStr, "<remote address>");
    cli.opt(&s_localAddr, "[local address]");
    cli.action(app);
    return appRun(argc, argv, kVersion, "tnet");
}
