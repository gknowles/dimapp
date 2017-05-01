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

template <typename Tag> class TimerList;

template <typename Tag = LinkDefault>
class ITimerListNotify 
    : public ListBaseLink<ITimerListNotify<Tag>> {
public:
    virtual ~ITimerListNotify() {}
    virtual void onTimer(TimePoint now, Tag) = 0;
private:
    friend class TimerList<Tag>;
    TimePoint m_lastTouched;
};

template <>
class ITimerListNotify<LinkDefault> 
    : public ListBaseLink<ITimerListNotify<LinkDefault>> {
public:
    virtual ~ITimerListNotify() {}
    virtual void onTimer(TimePoint now) = 0;
private:
    friend class TimerList<LinkDefault>;
    TimePoint m_lastTouched;
};


/****************************************************************************
*
*   TimerList
*
***/

template <typename Tag = LinkDefault>
class TimerList : ITimerNotify {
public:
    using value_type = ITimerListNotify<Tag>;

public:
    TimerList(Duration timeout, Duration minWait = kTimerDefaultMinWait);

    void setTimeout(
        Duration timeout, 
        Duration minWait = kTimerDefaultMinWait
    );

    void touch(value_type * notify);
    void unlink(value_type * notify);

    List<value_type> & values() { return m_nodes; }
    const List<value_type> & values() const { return m_nodes; }

private:
    Duration onTimer(TimePoint now) override;

    List<value_type> m_nodes;
    Duration m_timeout;
    Duration m_minWait;
};

//===========================================================================
template <typename Tag>
inline TimerList<Tag>::TimerList(Duration timeout, Duration minWait) {
    setTimeout(timeout, minWait);
}

//===========================================================================
template <typename Tag>
inline void TimerList<Tag>::setTimeout(Duration timeout, Duration minWait) {
    m_timeout = timeout;
    m_minWait = min(minWait, timeout);
    if (auto node = m_nodes.front())
        timerUpdate(this, node->m_lastTouched + timeout - Clock::now());
}

//===========================================================================
template <typename Tag>
inline void TimerList<Tag>::touch(ITimerListNotify<Tag> * notify) {
    notify->m_lastTouched = Clock::now();
    if (m_nodes.empty())
        timerUpdate(this, m_timeout);
    m_nodes.pushBack(notify);
}

//===========================================================================
template <typename Tag>
inline void TimerList<Tag>::unlink(ITimerListNotify<Tag> * notify) {
    m_nodes.unlink(notify);
}

//===========================================================================
template <typename Tag>
inline Duration TimerList<Tag>::onTimer(TimePoint now) {
    auto expire = now - m_timeout;
    while (auto node = m_nodes.front()) {
        auto wait = node->m_lastTouched - expire;
        if (wait > 0s)
            return wait;
        touch(node);
        node->onTimer(now, {});
    }
    return kTimerInfinite;
}

//===========================================================================
template <>
inline Duration TimerList<LinkDefault>::onTimer(TimePoint now) {
    auto expire = now - m_timeout;
    while (auto node = m_nodes.front()) {
        auto wait = node->m_lastTouched - expire;
        if (wait > (Duration) 0)
            return wait;
        touch(node);
        node->onTimer(now);
    }
    return kTimerInfinite;
}


} // namespace
