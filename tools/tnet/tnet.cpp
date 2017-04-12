// Copyright Glen Knowles 2015 - 2017.
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

const char kVersion[] = "1.0";

enum { kExitConnectFailed = EX__APPBASE, kExitDisconnect };


class SocketConn : public ISocketNotify, public IEndpointNotify {
    // ISocketNotify
    void onSocketConnect(const SocketInfo & info) override;
    void onSocketConnectFailed() override;
    void onSocketRead(const SocketData & data) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;

    // IEndpointNotify
    void onEndpointFound(const Endpoint * ptr, int count) override;

    unique_ptr<ConsoleScopedAttr> m_connected;
};

class ConsoleReader : public IFileReadNotify {
public:
    FileHandle m_device;
    bool m_isFile{false};

    bool QueryDestroy() const { return !m_device && !m_buffer; }
    void read(int64_t offset = 0);

private:
    bool onFileRead(string_view data, int64_t offset, FileHandle f) override;
    void onFileEnd(int64_t offset, FileHandle f) override;

    unique_ptr<SocketBuffer> m_buffer;
    int m_bytesRead{0};
};


/****************************************************************************
*
*   Variables
*
***/

static Endpoint s_localEnd;
static int s_cancelAddrId;
static ConsoleReader s_console;
static SocketConn s_socket;


/****************************************************************************
*
*   SocketConn
*
***/

//===========================================================================
void SocketConn::onEndpointFound(const Endpoint * ends, int count) {
    if (!count) {
        cout << "Host not found" << endl;
        appSignalShutdown(kExitConnectFailed);
    } else {
        cout << "Connecting on " << s_localEnd << " to " << *ends << endl;
        socketConnect(this, *ends, s_localEnd);
    }
}

//===========================================================================
void SocketConn::onSocketConnect(const SocketInfo & info) {
    m_connected = make_unique<ConsoleScopedAttr>(kConsoleGreen);
    cout << "Connected" << endl;
    s_console.read();
}

//===========================================================================
void SocketConn::onSocketConnectFailed() {
    cout << "Connect failed" << endl;
    appSignalShutdown(kExitConnectFailed);
}

//===========================================================================
void SocketConn::onSocketRead(const SocketData & data) {
    cout.write(data.data, data.bytes);
    cout.flush();
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


/****************************************************************************
*
*   ConsoleReader
*
***/

//===========================================================================
void ConsoleReader::read(int64_t offset) {
    assert(m_device);
    m_bytesRead = 0;
    m_buffer = socketGetBuffer();
    fileRead(this, m_buffer->data, m_buffer->len, m_device, offset);
}

//===========================================================================
bool ConsoleReader::onFileRead(
    string_view data, 
    int64_t offset,
    FileHandle f) {
    m_bytesRead = (int) data.size();
    socketWrite(&s_socket, move(m_buffer), m_bytesRead);
    // stop reading (return false) so we can get a new buffer
    return false;
}

//===========================================================================
void ConsoleReader::onFileEnd(int64_t offset, FileHandle f) {
    if (m_device) {
        if (m_isFile) {
            if (!m_bytesRead || (size_t) offset == fileSize(f)) {
                m_isFile = false;
                m_device = fileOpen("conin$", File::fReadOnly);
            }
        }
        read(offset);
    } else {
        m_buffer.reset();
    }
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool retry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool retry) {
    if (!retry) {
        fileClose(s_console.m_device);
        s_console.m_device = {};
        endpointCancelQuery(s_cancelAddrId);
        socketDisconnect(&s_socket);
    }

    if (socketGetMode(&s_socket) != ISocketNotify::kInactive
        || !s_console.QueryDestroy()
    ) {
        shutdownIncomplete();
    }
}


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public IAppNotify {
    void onAppRun() override;
};
} // namespace

//===========================================================================
void Application::onAppRun() {
    shutdownMonitor(&s_cleanup);

    Cli cli;
    cli.header("tnet v"s + kVersion + " (" __DATE__ ")");
    cli.versionOpt(kVersion);
    auto & remote = cli.opt<string>("<remote address>");
    cli.opt(&s_localEnd, "[local address]");
    if (!cli.parse(m_argc, m_argv))
        return appSignalUsageError();

    consoleEnableCtrlC();
    consoleEnableEcho(false);

    endpointQuery(&s_cancelAddrId, &s_socket, *remote, 23);

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD type = GetFileType(hIn);
    char inpath[MAX_PATH] = "conin$";
    if (type == FILE_TYPE_DISK) {
        s_console.m_isFile = true;
        GetFinalPathNameByHandle(
            hIn, inpath, (DWORD)size(inpath), FILE_NAME_OPENED);
    }
    cout << "Input from: " << inpath << endl;

    s_console.m_device = fileOpen(inpath, File::fReadOnly | File::fDenyNone);
    if (!s_console.m_device)
        return appSignalShutdown(EX_IOERR);
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

    Application app;
    return appRun(app, argc, argv);
}
