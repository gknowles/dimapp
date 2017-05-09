// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winapp.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const unsigned kUpdateIntervalMS = 1'000;


/****************************************************************************
*
*   Declarations
*
***/

const char kPerfWndClass[] = "DimPerfCounters";
const int kListId = 1;
const int kTimerId = 1;

const unsigned WM_USER_CLOSEWINDOW = WM_USER;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static HWND createList(HWND parent) {
    HWND wnd = CreateWindow(
        WC_LISTVIEW,
        "",
        WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | LVS_REPORT,
        0, 0, // x, y
        CW_USEDEFAULT, CW_USEDEFAULT, // cx, cy
        parent,
        (HMENU) (intptr_t) kListId,
        GetModuleHandle(NULL),
        NULL // user param to WM_CREATE
    );
    if (!wnd)
        logMsgCrash() << "CreateWindow(WC_LISTVIEW): " << WinError{};

    LVCOLUMN lc = {};
    lc.mask = LVCF_FMT | LVCF_TEXT;
    lc.fmt = LVCFMT_RIGHT;
    lc.pszText = (LPSTR) "Value";
    lc.iSubItem = 0;
    if (ListView_InsertColumn(wnd, 1, &lc) == -1)
        logMsgCrash() << "ListView_InsertColumn(1): " << WinError{};
    lc.fmt = LVCFMT_LEFT;
    lc.pszText = (LPSTR) "Name";
    lc.iSubItem = 1;
    if (ListView_InsertColumn(wnd, 2, &lc) == -1)
        logMsgCrash() << "ListView_InsertColumn(2): " << WinError{};
    return wnd;
}

//===========================================================================
static void moveList(HWND parent) {
    RECT rect;
    GetClientRect(parent, &rect);
    auto wnd = GetDlgItem(parent, kListId);
    MoveWindow(wnd, 0, 0, rect.right, rect.bottom, true);
    ListView_SetColumnWidth(wnd, 0, rect.right / 4);
    ListView_SetColumnWidth(wnd, 1, rect.right - rect.right / 4);
}

//===========================================================================
static void setItemText(HWND wnd, int item, int col, string_view text) {
    char tmp[256];
    LVITEM li = {};
    li.iItem = item;
    li.iSubItem = col;
    li.mask = LVIF_TEXT;
    li.pszText = (LPSTR) tmp;
    li.cchTextMax = sizeof(tmp);
    if (!ListView_GetItem(wnd, &li) || text != li.pszText)
        ListView_SetItemText(wnd, item, col, (LPSTR) text.data());
}

//===========================================================================
static void updateList(HWND parent) {
    auto wnd = GetDlgItem(parent, kListId);

    vector<PerfValue> vals;
    perfGetValues(vals);
    int cvals = (int) vals.size();

    LVITEM li = {};
    int num = ListView_GetItemCount(wnd);

    // add and/or update lines
    for (int i = 0; i < cvals; ++i) {
        if (i >= num) {
            li.iItem = i;
            ListView_InsertItem(wnd, &li);
        }
        setItemText(wnd, i, 0, vals[i].value);
        setItemText(wnd, i, 1, vals[i].name);
    }

    // remove extra lines
    for (int i = cvals; i < num; ++i) {
        ListView_DeleteItem(wnd, i);
    }
}

//===========================================================================
static LRESULT CALLBACK perfWindowProc(
    HWND wnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
) {
    switch (msg) {
    case WM_TIMER:
        updateList(wnd);
        return 0;

    case WM_CREATE:
        createList(wnd);
        return 0;

    case WM_SIZE:
        moveList(wnd);
        return 0;

    case WM_USER_CLOSEWINDOW:
        // used to close the window but not the app
        DestroyWindow(wnd);
        return 0;

    case WM_CLOSE:
        appSignalShutdown();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(wnd, msg, wparam, lparam);
}

//===========================================================================
static HWND createPerfWindow() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = perfWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = kPerfWndClass;
    if (!RegisterClassEx(&wc))
        logMsgCrash() << "RegisterClassEx: " << WinError{};

    long val = GetDialogBaseUnits();
    WORD diagX = (WORD) val;
    WORD diagY = val >> 16;

    auto wnd = CreateWindow(
        kPerfWndClass,
        "Perf Counters",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, // x, y
        50 * diagX, 50 * diagY, // cx, cy
        NULL, // parent
        NULL, // menu
        wc.hInstance,
        NULL // user param to WM_CREATE
    );
    if (!wnd)
        logMsgCrash() << "CreateWindow: " << WinError{};

    ShowWindow(wnd, SW_SHOWNORMAL);
    SetTimer(
        wnd, 
        kTimerId,
        kUpdateIntervalMS,
        NULL // timer proc
    );
    return wnd;
}


/****************************************************************************
*
*   Message loop thread
*
***/

namespace {
class MessageLoopTask : public ITaskNotify {
public:
    enum Authority {
        kCommandLine,
        kConfigFile,
        kShutdown,
    };
public:
    void enable(Authority from, bool enable);
    void onTask() override;
private:
    bool m_hasCommand{false};
    bool m_hasShutdown{false};
    bool m_fromConfig{false};
    HWND m_wnd{NULL};
};
} // namespace
static MessageLoopTask s_windowTask;
static TaskQueueHandle s_taskq;
static auto & s_cliEnable = Cli{}.opt<bool>("appwin")
    .desc("Show window UI")
    .after([](auto & cli, auto & opt, auto & val){
        if (*s_cliEnable)
            s_windowTask.enable(MessageLoopTask::kCommandLine, true);
        return true;
    });

//===========================================================================
void MessageLoopTask::enable(Authority auth, bool enable) {
    if (m_hasShutdown || auth == kShutdown) {
        assert(!enable);
        m_hasShutdown = true;
        enable = false;
    } else {
        if (auth == kCommandLine) {
            m_hasCommand = true;
        } else {
            assert(auth == kConfigFile);
            m_fromConfig = enable;
        }
        enable = s_cliEnable ? *s_cliEnable : m_fromConfig;
    }

    // don't show window before the command line has been processed
    if (!m_hasShutdown && !m_hasCommand)
        return;

    if (enable == (m_wnd != NULL)) 
        return;

    if (enable) {
        // start message loop task
        taskPush(s_taskq, *this);
    } else {
        KillTimer(m_wnd, kTimerId);
        PostMessage(m_wnd, WM_USER_CLOSEWINDOW, 0, 0);
    }
}

//===========================================================================
void MessageLoopTask::onTask() {
    m_wnd = createPerfWindow();

    MSG msg;
    while (BOOL rc = GetMessage(&msg, NULL, 0, 0)) {
        if (rc == -1)
            logMsgCrash() << "GetMessage: " << WinError{};
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClass(kPerfWndClass, GetModuleHandle(NULL));
    m_wnd = NULL;
}


/****************************************************************************
*
*   Config monitor 
*
***/

namespace {
class EnableNotify 
    : public IConfigNotify 
    , public ITaskNotify
{
    void onConfigChange(const XDocument & doc) override;
    void onTask() override;
};
} // namespace
static EnableNotify s_notify;

//===========================================================================
void EnableNotify::onConfigChange(const XDocument & doc) {
    bool enable = configUnsigned(doc, "EnableAppWindow");
    s_windowTask.enable(MessageLoopTask::kConfigFile, enable);
}

//===========================================================================
void EnableNotify::onTask() {
    if (appMode() == kRunRunning)
        s_windowTask.enable(MessageLoopTask::kCommandLine, *s_cliEnable);
}


/****************************************************************************
*
*   Shutdown notify
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
    if (firstTry)
        return shutdownIncomplete();

    s_windowTask.enable(MessageLoopTask::kShutdown, false);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::winAppInitialize() {
    shutdownMonitor(&s_cleanup);
    s_windowTask = {};
    s_taskq = taskCreateQueue("Message Loop", 1);
    iAppPushStartupTask(s_notify);
}

//===========================================================================
void Dim::winAppConfigInitialize() {
    configMonitor("app.xml", &s_notify);
}
