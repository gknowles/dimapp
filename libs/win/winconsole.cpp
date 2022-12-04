// Copyright Glen Knowles 2016 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// winconsole.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static vector<WORD> s_consoleAttrs;
static bool s_catchEnabled = false;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static BOOL WINAPI controlCallback(DWORD ctrl) {
    if (s_catchEnabled) {
        switch (ctrl) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            appSignalShutdown(EX_IOERR);
            return true;
        }
    }

    return false;
}

//===========================================================================
static void enableConsoleFlags(DWORD handleOrd, bool enable, DWORD flags) {
    HANDLE f = GetStdHandle(handleOrd);
    DWORD mode = 0;
    if (!GetConsoleMode(f, &mode))
        return;
    if (enable) {
        mode |= flags;
    } else {
        mode &= ~flags;
    }
    if (!SetConsoleMode(f, mode))
        logMsgFatal() << "SetConsoleMode(): " << WinError{};
}

//===========================================================================
static bool getStdInfo(int * dst, const wchar_t ** dev, DWORD nstd) {
    switch (nstd) {
    case STD_INPUT_HANDLE: *dst = 0; *dev = L"conin$"; break;
    case STD_OUTPUT_HANDLE: *dst = 1; *dev = L"conout$"; break;
    case STD_ERROR_HANDLE: *dst = 2; *dev = L"conout$"; break;
    default:
        assert(!"Invalid nStdHandle value");
        return false;
    }
    return true;
}


/****************************************************************************
*
*   ConsoleScopedAttr
*
***/

static atomic<unsigned> s_originalAttr;
static atomic<int> s_numAttrScopes;

//===========================================================================
constexpr int findAttr(ConsoleAttr attr) {
    switch (attr) {
    case kConsoleNormal: // normal white
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    case kConsoleCheer: // bright green
        return FOREGROUND_INTENSITY | FOREGROUND_GREEN;
    case kConsoleNote: // bright cyan
        return FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE;
    case kConsoleWarn: // bright yellow
        return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
    case kConsoleError: // bright red
        return FOREGROUND_INTENSITY | FOREGROUND_RED;
    default:
        return -1;
    }
}

//===========================================================================
ConsoleScopedAttr::ConsoleScopedAttr(ConsoleAttr attr) {
    if (!consoleAttached()) {
        m_active = false;
        return;
    }

    // save console text attributes
    m_rawAttrs = consoleRawAttr();
    if (s_numAttrScopes++ == 0)
        s_originalAttr = m_rawAttrs;

    if (auto raw = findAttr(attr); raw != -1)
        consoleRawAttr(raw);
}

//===========================================================================
ConsoleScopedAttr::~ConsoleScopedAttr() {
    if (m_active) {
        if (--s_numAttrScopes == 0) {
            // Ensure that the last scope always restores the console to its
            // original state, even with interleaving scopes.
            m_rawAttrs = s_originalAttr;
        }
        consoleRawAttr(m_rawAttrs);
    }
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::winConsoleInitialize() {
    // set control-c handler
    SetConsoleCtrlHandler(&controlCallback, true);
    enableConsoleFlags(
        STD_OUTPUT_HANDLE,
        true,
        ENABLE_VIRTUAL_TERMINAL_PROCESSING
    );
}


/****************************************************************************
*
*   Console manipulation
*
***/

//===========================================================================
bool Dim::consoleAttached() {
    return GetConsoleWindow() != NULL;
}

//===========================================================================
void Dim::consoleEnableEcho(bool enable) {
    enableConsoleFlags(
        STD_INPUT_HANDLE,
        enable,
        ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT
    );
}

//===========================================================================
void Dim::consoleCatchCtrlC(bool enable) {
    s_catchEnabled = enable;
}

//===========================================================================
void Dim::consoleRedoLine() {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgFatal() << "GetConsoleScreenBufferInfo: " << GetLastError();
    }
    if (info.dwCursorPosition.Y != 0) {
        info.dwCursorPosition.Y -= 1;
        info.dwCursorPosition.X = 0;
        SetConsoleCursorPosition(hOutput, info.dwCursorPosition);
    }
    DWORD count;
    FillConsoleOutputCharacter(
        hOutput,
        ' ',
        info.dwMaximumWindowSize.X,
        info.dwCursorPosition,
        &count
    );
}

//===========================================================================
unsigned Dim::consoleWidth() {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgFatal() << "GetConsoleScreenBufferInfo: " << GetLastError();
    }
    return info.dwSize.X;
}

//===========================================================================
unsigned Dim::consoleRawAttr() {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgFatal() << "GetConsoleScreenBufferInfo: " << WinError{};
    }
    return info.wAttributes;
}

//===========================================================================
void Dim::consoleRawAttr(unsigned newAttr) {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hOutput, (WORD) newAttr);
}


/****************************************************************************
*
*   Reassign CRT lowio handles to duplicates so they can be safely closed.
*
*   The first call to FreeConsole() from a console application can not be
*   done safely -- using documented APIs -- without breaking the standard
*   C low IO descriptors (0, 1, and 2).
*
*   The standard descriptors are initialized by the MSVC CRT to own the
*   underlying Windows console handles. However, the underlying console also
*   owns them. This means that when the application is only thing running in
*   the console they get closed twice, once by closing the CRT handles and
*   again by FreeConsole. The second CloseHandle, quite properly, raises an
*   invalid handle exception when in the debugger. The danger is that the OS
*   could reuse the handle between the calls and we're now closing the handle
*   to some unrelated thing.
*
*   After the first time this is no longer a problem because we dup the
*   handles when attaching the new console. The most frequent manifestation
*   is when the console application is run in a new console, and it's not a
*   problem for windows apps since they don't start with a console.
*
*   There is no supported way to do any of the following:
*       - close the CRT descriptors without closing the os handles
*       - attach the CRT descriptors to new os handles without first closing
*           them (i.e. _dup2 implicitly closes the target).
*       - free the console without closing the os handles (SetStdHandle does
*           not affect the internal list of handles).
*       - register an initialization function in a .CRT section to
*           duplicate the handles before the CRT copies them. (the standard
*           descriptors are initialized before registered init functions run).
*
*   Which leaves the uncomfortable options:
*       - custom entry point that dups the handles and only then calls
*           calls mainCRTStartup.
*       - call the undocumented _free_osfhnd function.
*
*   TL;DR
*   We put a function, to duplicate console handles into the standard file
*   descriptors (using the undocumented _free_osfhnd), in the non-standard
*   .CRT$XIU section so it runs before static constructors.
*
***/

extern "C" int __cdecl _free_osfhnd(int);

//===========================================================================
extern "C" static int reassignCrtHandles() {
    auto hproc = GetCurrentProcess();
    for (auto nstd : {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
        int fd;
        const wchar_t * dev;
        getStdInfo(&fd, &dev, nstd);

        auto osf = GetStdHandle(nstd);
        if (!osf || osf == INVALID_HANDLE_VALUE)
            continue;
        if (GetFileType(osf) != FILE_TYPE_CHAR)
            continue;

        HANDLE osfNew;
        if (!DuplicateHandle(
            hproc,
            osf,
            hproc,
            &osfNew,
            0,      // access
            false,  // inherit
            DUPLICATE_SAME_ACCESS
        )) {
            continue;
        }

        _free_osfhnd(fd);
        auto fdNew = _open_osfhandle((intptr_t) osfNew, _O_TEXT);
        if (fd != fdNew) {
            if (_dup2(fdNew, fd) == -1)
                continue;
            _close(fdNew);
        }
    }
    return 0;
}

// Put function pointer in section that will be automatically called by the
// C runtime. ".CRT$XIU" is run after the C runtime is initialized (XIC and
// XIL) but before C++ namespace constructors (XC*).
#pragma section(".CRT$XIU", long, read)
#pragma data_seg(push)
#pragma data_seg(".CRT$XIU")
static auto s_reassignHandles = reassignCrtHandles;
#pragma data_seg(pop)


/****************************************************************************
*
*   Console handle
*
***/

//===========================================================================
// Attach existing console handle to standard C file descriptor (stdin,
// stdout, or stderr).
static void attachStd(DWORD nstd) {
    int dst;
    const wchar_t * dev;
    if (!getStdInfo(&dst, &dev, nstd))
        return;

    // Get existing handle
    int fd = -1;
    auto osfOld = GetStdHandle(nstd);
    if (!osfOld || osfOld == INVALID_HANDLE_VALUE) {
        logMsgFatal() << "GetStdHandle(" << dst << '/' << dev << "): "
            << errno << ", " << _doserrno;
        return;
    }
    // Duplicate it because the console subsystem owns that handle and will
    // close it in FreeConsole.
    HANDLE osfNew;
    if (!DuplicateHandle(
        GetCurrentProcess(),
        osfOld,
        GetCurrentProcess(),
        &osfNew,
        0,
        false, // inherit
        DUPLICATE_SAME_ACCESS
    )) {
        logMsgFatal() << "DuplicateHandle(" << dst << '/' << dev << "): "
            << WinError{};
        return;
    }
    // Create file descriptor from the new handle, the fd will own it.
    fd = _open_osfhandle((intptr_t) osfNew, _O_TEXT);
    if (fd == -1) {
        logMsgFatal() << "open_osfhandle(" << dst << '/' << dev << "): "
            << errno << ", " << _doserrno;
        return;
    }
    // Assign the new descriptor to the standard value (0, 1, or 2) for stdin,
    // stdout, or stderr. Skip if fd and dst are already equal, which can
    // happen if the target fd was previously closed.
    if (fd != dst) {
        if (_dup2(fd, dst) == -1)
            logMsgFatal() << "dup2(" << dst << '/' << dev << "): "
                << errno << ", " << _doserrno;
        _close(fd);
    }
    SetStdHandle(nstd, (HANDLE) _get_osfhandle(dst));
}

//===========================================================================
static void resetStd(DWORD nstd) {
    if (!consoleAttached()) {
        // process isn't attached to a console
        return;
    }
    HANDLE osf = GetStdHandle(nstd);
    DWORD mode = 0;
    if (GetConsoleMode(osf, &mode))
        return;
    int dst;
    const wchar_t * dev;
    if (!getStdInfo(&dst, &dev, nstd))
        return;

    // Create explicit handle to console and attach it to standard file
    // descriptor.
    osf = CreateFileW(
        dev,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, // security attributes
        OPEN_EXISTING,
        0, // flags and attributes
        NULL // template file
    );
    if (osf == INVALID_HANDLE_VALUE) {
        logMsgError() << "CreateFileW(" << dev << "): " << WinError{};
        return;
    }
    if (!SetStdHandle(nstd, osf)) {
        logMsgError() << "SetStdHandle(in): " << WinError{};
    } else {
        attachStd(nstd);
    }
}

//===========================================================================
void Dim::consoleResetStdin() {
    resetStd(STD_INPUT_HANDLE);
}

//===========================================================================
void Dim::consoleResetStdout() {
    resetStd(STD_OUTPUT_HANDLE);
}

//===========================================================================
void Dim::consoleResetStderr() {
    resetStd(STD_ERROR_HANDLE);
}

//===========================================================================
bool Dim::consoleAttach(intptr_t pid) {
    _close(0);
    _close(1);
    _close(2);
    FreeConsole();

    bool success = true;
    if (!AttachConsole((DWORD) pid)) {
        success = false;
        WinError err;
        if (err == ERROR_INVALID_HANDLE) {
            // Process supposedly attached to the target console doesn't
            // exist or doesn't have a console.
        } else {
            logMsgError() << "AttachConsole(pid): " << err;
        }
        if (!AllocConsole()) {
            err.set();
            logMsgFatal() << "AllocConsole: " << err;
        }
    }
    if (consoleAttached()) {
        attachStd(STD_INPUT_HANDLE);
        attachStd(STD_OUTPUT_HANDLE);
        attachStd(STD_ERROR_HANDLE);
    }
    return success;
}
