// log.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

namespace {
class DefaultNotify : public ILogNotify {
    void onLog(LogType type, std::string_view msg) override;
};
} // namespace

static DefaultNotify s_fallback;
static vector<ILogNotify *> s_notifiers;
static ILogNotify * s_defaultNotify{&s_fallback};
static atomic<int> s_counts[kLogTypes];


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void LogMsg(LogType type, string_view msg) {
    s_counts[type] += 1;

    if (!msg.empty() && msg.back() == '\n')
        msg.remove_suffix(1);
    if (s_notifiers.empty()) {
        s_defaultNotify->onLog(type, msg);
    } else {
        for (auto && notify : s_notifiers) {
            notify->onLog(type, msg);
        }
    }

    if (type == kLogTypeCrash)
        abort();
}


/****************************************************************************
*
*   DefaultNotify
*
***/

//===========================================================================
void DefaultNotify::onLog(LogType type, std::string_view msg) {
    cout.write(msg.data(), msg.size());
}


/****************************************************************************
*
*   Log
*
***/

//===========================================================================
Detail::Log::Log(LogType type)
    : m_type(type) {}

//===========================================================================
Detail::Log::Log(Log && from)
    : ostringstream(static_cast<ostringstream &&>(from))
    , m_type(from.m_type) {}

//===========================================================================
Detail::Log::~Log() {
    auto s = str();
    LogMsg(m_type, s);
}


/****************************************************************************
*
*   LogCrash
*
***/

//===========================================================================
Detail::LogCrash::~LogCrash() {}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
void Dim::logSetDefaultNotify(ILogNotify * notify) {
    s_defaultNotify = notify ? notify : &s_fallback;
}

//===========================================================================
void Dim::logAddNotify(ILogNotify * notify) {
    s_notifiers.push_back(notify);
}

//===========================================================================
Detail::Log Dim::logMsgDebug() {
    return kLogTypeDebug;
}

//===========================================================================
Detail::Log Dim::logMsgInfo() {
    return kLogTypeInfo;
}

//===========================================================================
Detail::Log Dim::logMsgError() {
    return kLogTypeError;
}

//===========================================================================
Detail::LogCrash Dim::logMsgCrash() {
    return kLogTypeCrash;
}

//===========================================================================
void Dim::logParseError(
    string_view msg,
    string_view objname,
    size_t pos,
    string_view source) {

    auto lineNum = 1 + count(source.begin(), source.begin() + pos, '\n');
    logMsgError() << objname << "(" << lineNum << "): " << msg;

    bool leftTrunc = false;
    bool rightTrunc = false;
    size_t first = source.find_last_of('\n', pos);
    first = (first == string::npos) ? 0 : first + 1;
    if (pos - first > 50) {
        leftTrunc = true;
        first = pos - 50;
    }
    size_t last = source.find_first_of('\n', pos);
    last = source.find_last_not_of(" \t\r\n", last);
    last = (last == string::npos) ? source.size() : last + 1;
    if (last - first > 78) {
        rightTrunc = true;
        last = first + 78;
    }
    size_t len = last - first;
    auto line = string(source.substr(first, len));
    for (char & ch : line) {
        if (iscntrl((unsigned char)ch))
            ch = ch == '\t' ? ' ' : '.';
    }
    logMsgInfo() << string(leftTrunc * 3, '.') << line
                 << string(rightTrunc * 3, '.');
    logMsgInfo() << string(pos - first + leftTrunc * 3, ' ') << '^';
}

//===========================================================================
int Dim::logGetMsgCount(LogType type) {
    assert(type < kLogTypes);
    return s_counts[type];
}
