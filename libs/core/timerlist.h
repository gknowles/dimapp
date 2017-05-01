// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// timerlist.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/list.h"
#include "core/timer.h"

namespace Dim {


/****************************************************************************
*
*   Constants
*
***/

const Duration kTimerDefaultMinWait = (std::chrono::seconds) 2;


/****************************************************************************
*
*   Implemented by clients
*
***/

template <typename T, typename Tag = LinkDefault> class TimerList;

template <typename T, typename Tag = LinkDefault>
class ITimerListNotify : public ListBaseLink<T, Tag> {
public:
    virtual ~ITimerListNotify() {}
    virtual void onTimer(TimePoint now, Tag tag = {}) = 0;
private:
    friend class TimerList<T, Tag>;
    TimePoint m_lastTouched;
};

template <typename T>
class ITimerListNotify<T, LinkDefault> : public ListBaseLink<T, LinkDefault> {
public:
    virtual ~ITimerListNotify() {}
    virtual void onTimer(TimePoint now) = 0;
private:
    friend class TimerList<T, LinkDefault>;
    TimePoint m_lastTouched;
};


/****************************************************************************
*
*   TimerList
*
***/

template <typename T, typename Tag>
class TimerList : ITimerNotify {
public:
    TimerList(Duration timeout, Duration minWait = kTimerDefaultMinWait);

    void setTimeout(
        Duration timeout, 
        Duration minWait = kTimerDefaultMinWait
    );

    void touch(T * notify);
    void unlink(T * notify);

    List<T, Tag> & values() { return m_nodes; }
    const List<T, Tag> & values() const { return m_nodes; }

private:
    Duration onTimer(TimePoint now) override;

    List<T, Tag> m_nodes;
    Duration m_timeout;
    Duration m_minWait;
};

//===========================================================================
template <typename T, typename Tag>
inline TimerList<T,Tag>::TimerList(Duration timeout, Duration minWait) {
    static_assert(std::is_base_of_v<ITimerListNotify<T,Tag>, T>);
    setTimeout(timeout, minWait);
}

//===========================================================================
template <typename T, typename Tag>
inline void TimerList<T,Tag>::setTimeout(Duration timeout, Duration minWait) {
    m_timeout = timeout;
    m_minWait = min(minWait, timeout);
    if (auto node = m_nodes.front())
        timerUpdate(this, node->m_lastTouched + timeout - Clock::now());
}

//===========================================================================
template <typename T, typename Tag>
inline void TimerList<T,Tag>::touch(T * notify) {
    notify->m_lastTouched = Clock::now();
    if (m_nodes.empty())
        timerUpdate(this, m_timeout);
    m_nodes.pushBack(notify);
}

//===========================================================================
template <typename T, typename Tag>
inline void TimerList<T,Tag>::unlink(T * notify) {
    m_nodes.unlink(notify);
}

//===========================================================================
template <typename T, typename Tag>
inline Duration TimerList<T,Tag>::onTimer(TimePoint now) {
    auto expire = now - m_timeout;
    while (auto node = m_nodes.front()) {
        auto wait = node->m_lastTouched - expire;
        if (wait > 0s)
            return wait;
        touch(node);
        static_cast<ITimerListNotify<T,Tag>*>(node)->onTimer(now);
    }
    return kTimerInfinite;
}

} // namespace
