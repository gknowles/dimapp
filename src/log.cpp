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

static vector<ILogNotify *> s_notifiers;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void LogMsg(LogType type, const string & msg) {
    if (s_notifiers.empty()) {
        cout << msg << endl;
    } else {
        for (auto && notify : s_notifiers) {
            notify->onLog(type, msg);
        }
    }

    if (type == kLogCrash)
        abort();
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
Detail::Log::~Log() {
    LogMsg(m_type, str());
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
void Dim::logAddNotify(ILogNotify * notify) {
    s_notifiers.push_back(notify);
}

//===========================================================================
Detail::Log Dim::logMsgDebug() {
    return kLogDebug;
}

//===========================================================================
Detail::Log Dim::logMsgInfo() {
    return kLogInfo;
}

//===========================================================================
Detail::Log Dim::logMsgError() {
    return kLogError;
}

//===========================================================================
Detail::LogCrash Dim::logMsgCrash() {
    return kLogCrash;
}

//===========================================================================
void Dim::logParseError(
    const std::string & msg, 
    const std::string & path,
    size_t pos, 
    const std::string & source) {

    auto lineNum =
        1 + count(source.begin(), source.begin() + pos, '\n');
    logMsgError() << path << "(" << lineNum << "): " << msg;

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
    string line = source.substr(first, len);
    for (auto && ch : line) {
        if (iscntrl(ch))
            ch = '.';
    }
    logMsgInfo() << string(leftTrunc * 3, '.')
        << line
        << string(rightTrunc * 3, '.');
    logMsgInfo() << string(pos - first + leftTrunc * 3, ' ') << '^';
}
