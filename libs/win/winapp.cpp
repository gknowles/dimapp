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


/****************************************************************************
*
*   Variables
*
***/

static HWND s_wnd;
static HWND s_listWnd;


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
static void moveList(HWND wnd, const RECT & rect) {
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
static void updateList(HWND wnd) {
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
    RECT rect;

    switch (msg) {
    case WM_TIMER:
        updateList(s_listWnd);
        return 0;

    case WM_CREATE:
        s_listWnd = createList(wnd);
        return 0;

    case WM_SIZE:
        GetClientRect(wnd, &rect);
        moveList(s_listWnd, rect);
        return 0;

    case WM_DESTROY:
        s_wnd = NULL;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(wnd, msg, wparam, lparam);
}

//===========================================================================
static void createPerfWindow() {
    long val = GetDialogBaseUnits();
    WORD diagX = (WORD) val;
    WORD diagY = val >> 16;

    s_wnd = CreateWindow(
        kPerfWndClass,
        "Perf Counters",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, // x, y
        50 * diagX, 50 * diagY, // cx, cy
        NULL, // parent
        NULL, // menu
        GetModuleHandle(NULL),
        NULL // user param to WM_CREATE
    );
    if (!s_wnd)
        logMsgCrash() << "CreateWindow: " << WinError{};

    ShowWindow(s_wnd, SW_SHOWNORMAL);
    SetTimer(
        s_wnd, 
        kTimerId,
        kUpdateIntervalMS,
        NULL // timer proc
    );
}


/****************************************************************************
*
*   Message loop thread
*
***/

namespace {
class MessageLoopTask : public ITaskNotify {
public:
    void enable(bool enable, bool neverAgain = false);
    void onTask() override;
private:
    bool m_neverAgain{false};
};
} // namespace
static MessageLoopTask s_windowTask;
static TaskQueueHandle s_taskq;

//===========================================================================
void MessageLoopTask::enable(bool enable, bool neverAgain) {
    if (neverAgain) {
        assert(!enable);
        m_neverAgain = true;
    }
    if (enable == (s_wnd != NULL)) 
        return;

    if (enable && !m_neverAgain) {
        // start message loop task
        taskPush(s_taskq, *this);
    } else {
        KillTimer(s_wnd, kTimerId);
        PostMessage(s_wnd, WM_CLOSE, 0, 0);
    }
}

//===========================================================================
void MessageLoopTask::onTask() {
    createPerfWindow();

    MSG msg;
    while (BOOL rc = GetMessage(&msg, NULL, 0, 0)) {
        if (rc == -1)
            logMsgCrash() << "GetMessage: " << WinError{};
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


/****************************************************************************
*
*   Config monitor 
*
***/

namespace {
class AppXmlNotify : public IConfigNotify {
    void onConfigChange(const XDocument & doc) override;
};
} // namespace
static AppXmlNotify s_appXml;

//===========================================================================
void AppXmlNotify::onConfigChange(const XDocument & doc) {
    bool enable = configUnsigned(doc, "EnableAppWindow");
    s_windowTask.enable(enable);
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

    s_windowTask.enable(false, true);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::winAppInitialize() {
    shutdownMonitor(&s_cleanup);

    s_taskq = taskCreateQueue("Message Loop", 1);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = perfWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = kPerfWndClass;
    if (!RegisterClassEx(&wc))
        logMsgCrash() << "RegisterClassEx: " << WinError{};
}

//===========================================================================
void Dim::winAppConfigInitialize() {
    configMonitor("app.xml", &s_appXml);
}
