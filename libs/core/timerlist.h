// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// timerlist.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/list.h"
#include "core/timer.h"

#include <chrono>

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

template <typename Tag = LinkDefault>
class ITimerListNotify : public ListBaseLink<Tag> {
public:
    virtual ~ITimerListNotify() = default;
    virtual void onTimer(TimePoint now, Tag * tag = nullptr) = 0;
private:
    template <typename T, typename Tag> friend class TimerList;
    TimePoint m_lastTouched;
};

template <>
class ITimerListNotify<LinkDefault> : public ListBaseLink<LinkDefault> {
public:
    virtual ~ITimerListNotify() = default;
    virtual void onTimer(TimePoint now) = 0;
private:
    template <typename T, typename Tag> friend class TimerList;
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
    static_assert(std::is_base_of_v<ITimerListNotify<Tag>, T>);
    setTimeout(timeout, minWait);
}

//===========================================================================
template <typename T, typename Tag>
inline void TimerList<T,Tag>::setTimeout(Duration timeout, Duration minWait) {
    m_timeout = timeout;
    m_minWait = min(minWait, timeout);
    if (auto node = m_nodes.front()) {
        auto notify = static_cast<ITimerListNotify<Tag>*>(node);
        timerUpdate(this, notify->m_lastTouched + timeout - Clock::now());
    }
}

//===========================================================================
template <typename T, typename Tag>
inline void TimerList<T,Tag>::touch(T * node) {
    auto notify = static_cast<ITimerListNotify<Tag>*>(node);
    notify->m_lastTouched = Clock::now();
    if (m_nodes.empty())
        timerUpdate(this, m_timeout);
    m_nodes.link(node);
}

//===========================================================================
template <typename T, typename Tag>
inline void TimerList<T,Tag>::unlink(T * node) {
    m_nodes.unlink(node);
}

//===========================================================================
template <typename T, typename Tag>
inline Duration TimerList<T,Tag>::onTimer(TimePoint now) {
    auto expire = now - m_timeout;
    while (auto node = m_nodes.front()) {
        auto notify = static_cast<ITimerListNotify<Tag>*>(node);
        auto wait = notify->m_lastTouched - expire;
        if (wait > (std::chrono::seconds) 0)
            return wait;
        touch(node);
        notify->onTimer(now);
    }
    return kTimerInfinite;
}

} // namespace
