// main.cpp - tnet
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

enum { kExitConnectFailed = kExitFirstAvailable, kExitDisconnect };


/****************************************************************************
*
*   Variables
*
***/

static Endpoint s_localEnd;
static int s_cancelAddrId;


/****************************************************************************
*
*   SocketConn
*
***/

class SocketConn : public ISocketNotify, public IEndpointNotify {
    // ISocketNotify
    void onSocketConnect(const SocketConnectInfo &info) override;
    void onSocketConnectFailed() override;
    void onSocketRead(const SocketData &data) override;
    void onSocketDisconnect() override;

    // IDimEndpointNotify
    void onEndpointFound(Endpoint *ends, int count) override;

    unique_ptr<ConsoleScopedAttr> m_connected;
};
static SocketConn s_socket;

//===========================================================================
void SocketConn::onEndpointFound(Endpoint *ends, int count) {
    if (!count) {
        cout << "Host not found" << endl;
        appSignalShutdown(kExitConnectFailed);
    } else {
        cout << "Connecting on " << s_localEnd << " to " << *ends << endl;
        socketConnect(this, *ends, s_localEnd);
    }
}

//===========================================================================
void SocketConn::onSocketConnect(const SocketConnectInfo &info) {
    m_connected = make_unique<ConsoleScopedAttr>(kConsoleGreen);
    cout << "Connected" << endl;
}

//===========================================================================
void SocketConn::onSocketConnectFailed() {
    cout << "Connect failed" << endl;
    appSignalShutdown(kExitConnectFailed);
}

//===========================================================================
void SocketConn::onSocketRead(const SocketData &data) {
    cout.write(data.data, data.bytes);
    cout.flush();
}

//===========================================================================
void SocketConn::onSocketDisconnect() {
    m_connected.reset();
    appSignalShutdown(kExitDisconnect);
}


/****************************************************************************
*
*   ConsoleReader
*
***/

class ConsoleReader : public IFileReadNotify {
  public:
    unique_ptr<SocketBuffer> m_buffer;
    unique_ptr<IFile> m_file;

    bool QueryDestroy() const { return !m_file && !m_buffer; }

    bool
    onFileRead(char *data, int bytes, int64_t offset, IFile *file) override;
    void onFileEnd(int64_t offset, IFile *file) override;
};
static ConsoleReader s_console;

//===========================================================================
bool ConsoleReader::onFileRead(
    char *data, int bytes, int64_t offset, IFile *file) {
    socketWrite(&s_socket, move(m_buffer), bytes);
    // stop reading (return false) so we can get a new buffer
    return false;
}

//===========================================================================
void ConsoleReader::onFileEnd(int64_t offset, IFile *file) {
    if (m_file) {
        m_buffer = socketGetBuffer();
        fileRead(this, m_buffer->data, m_buffer->len, file);
    } else {
        m_buffer.reset();
    }
}


/****************************************************************************
*
*   MainShutdown
*
***/

class MainShutdown : public IAppShutdownNotify {
    void onAppStartClientCleanup() override;
    bool onAppQueryClientDestroy() override;
};
static MainShutdown s_cleanup;

//===========================================================================
void MainShutdown::onAppStartClientCleanup() {
    s_console.m_file.reset();
    endpointCancelQuery(s_cancelAddrId);
    socketDisconnect(&s_socket);
}

//===========================================================================
bool MainShutdown::onAppQueryClientDestroy() {
    if (socketGetMode(&s_socket) != ISocketNotify::kInactive ||
        !s_console.QueryDestroy()) {
        return appQueryDestroyFailed();
    }
    return true;
}


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public ITaskNotify {
    int m_argc;
    char **m_argv;

  public:
    Application(int argc, char *argv[]);
    void onTask() override;
};
} // namespace

//===========================================================================
Application::Application(int argc, char *argv[])
    : m_argc(argc)
    , m_argv(argv) {}

//===========================================================================
void Application::onTask() {
    appMonitorShutdown(&s_cleanup);

    if (m_argc < 2) {
        cout << "tnet v1.0 (" __DATE__ ")\n"
             << "usage: tnet <remote address> [<local address>]\n";
        return appSignalShutdown(kExitBadArgs);
    }

    consoleEnableEcho(false);

    if (m_argc > 2)
        parse(&s_localEnd, m_argv[2], 0);

    endpointQuery(&s_cancelAddrId, &s_socket, m_argv[1], 23);

    fileOpen(s_console.m_file, "conin$", IFile::kReadWrite);
    s_console.m_buffer = socketGetBuffer();
    fileRead(
        &s_console,
        s_console.m_buffer->data,
        s_console.m_buffer->len,
        s_console.m_file.get());
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char *argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Application app(argc, argv);
    return appRun(app);
}
