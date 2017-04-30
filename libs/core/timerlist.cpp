// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// timerlist.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   TimerList
*
***/

//===========================================================================
TimerList::TimerList(Duration timeout, Duration minWait) {
    setTimeout(timeout, minWait);
}

//===========================================================================
void TimerList::setTimeout(Duration timeout, Duration minWait) {
    m_timeout = timeout;
    m_minWait = min(minWait, timeout);
    if (auto node = m_nodes.front())
        timerUpdate(this, node->m_lastTouched + timeout - Clock::now());
}

//===========================================================================
void TimerList::touch(ITimerListNotify * notify) {
    notify->m_lastTouched = Clock::now();
    if (m_nodes.empty())
        timerUpdate(this, m_timeout);
    m_nodes.pushBack(notify);
}

//===========================================================================
void TimerList::unlink(ITimerListNotify * notify) {
    m_nodes.unlink(notify);
}

//===========================================================================
Duration TimerList::onTimer(TimePoint now) {
    auto expire = now - m_timeout;
    while (auto node = m_nodes.front()) {
        auto wait = node->m_lastTouched - expire;
        if (wait > 0s)
            return wait;
        touch(node);
        node->onTimer(now);
    }
    return kTimerInfinite;
}
