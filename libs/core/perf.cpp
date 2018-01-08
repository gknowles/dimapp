// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// perf.cpp - dim core
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

struct PerfInfo {
    // mutex only needed for the vector itself.
    shared_mutex mut;
    vector<unique_ptr<PerfCounterBase>> counters;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

// Number of counters declared during static initialization (or anytime
// before appRun).
static size_t s_numStatic;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static PerfInfo & getInfo () {
    static PerfInfo info;
    return info;
}

//===========================================================================
template <typename T>
static PerfCounter<T> & perf(string_view name) {
    auto & info = getInfo();
    unique_lock<shared_mutex> lk{info.mut};
    info.counters.push_back(make_unique<PerfCounter<T>>());
    auto & cnt = static_cast<PerfCounter<T>&>(*info.counters.back());
    cnt.name = name;
    return cnt;
}

//===========================================================================
template <typename T>
static PerfFunc<T> & perf(string_view name, function<T()> && fn) {
    auto & info = getInfo();
    unique_lock<shared_mutex> lk{info.mut};
    info.counters.push_back(make_unique<PerfFunc<T>>());
    auto & cnt = static_cast<PerfFunc<T>&>(*info.counters.back());
    cnt.name = name;
    cnt.fn = move(fn);
    return cnt;
}

//===========================================================================
template <typename T>
static void valueToString(std::string & out, T val, bool pretty) {
    auto str = StrFrom<T>{val};
    if (!pretty) {
        out = str;
        return;
    }

    auto v = string_view{str};
    auto num = (v.size() - 1) % 3 + 1;
    out.resize(v.size() + (v.size() - num) / 3);
    auto optr = out.data();
    auto ptr = v.data();
    auto eptr = ptr + v.size();
    switch (num) {
        case 3: *optr++ = *ptr++;
        case 2: *optr++ = *ptr++;
        case 1: *optr++ = *ptr++;
    }
    while (ptr != eptr) {
        *optr++ = ',';
        *optr++ = *ptr++;
        *optr++ = *ptr++;
        *optr++ = *ptr++;
    }
}


/****************************************************************************
*
*   PerfCounter
*
***/

template PerfCounter<int>;
template PerfCounter<unsigned>;
template PerfCounter<float>;

//===========================================================================
template<typename T>
inline double PerfCounter<T>::toDouble () const {
    return (double) *this;
}

//===========================================================================
template<typename T>
inline void PerfCounter<T>::toString (std::string & out, bool pretty) const {
    valueToString(out, (T) *this, pretty);
}


/****************************************************************************
*
*   PerfFunc
*
***/

template PerfFunc<int>;
template PerfFunc<unsigned>;
template PerfFunc<float>;

//===========================================================================
template<typename T>
inline double PerfFunc<T>::toDouble () const {
    return (double) fn();
}

//===========================================================================
template<typename T>
inline void PerfFunc<T>::toString (std::string & out, bool pretty) const {
    valueToString(out, fn(), pretty);
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iPerfInitialize() {
    auto & info = getInfo();
    unique_lock<shared_mutex> lk{info.mut};
    s_numStatic = info.counters.size();
}

//===========================================================================
void Dim::iPerfDestroy() {
    auto & info = getInfo();
    unique_lock<shared_mutex> lk{info.mut};
    assert(info.counters.size() >= s_numStatic);
    info.counters.resize(s_numStatic);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
PerfCounter<int> & Dim::iperf(string_view name) {
    return perf<int>(name);
}

//===========================================================================
PerfCounter<unsigned> & Dim::uperf(string_view name) {
    return perf<unsigned>(name);
}

//===========================================================================
PerfCounter<float> & Dim::fperf(string_view name) {
    return perf<float>(name);
}

//===========================================================================
PerfFunc<int> & Dim::iperf(string_view name, function<int()> fn) {
    return perf<int>(name, move(fn));
}

//===========================================================================
PerfFunc<unsigned> & Dim::uperf(
    string_view name,
    function<unsigned()> fn
) {
    return perf<unsigned>(name, move(fn));
}

//===========================================================================
PerfFunc<float> & Dim::fperf(string_view name, function<float()> fn) {
    return perf<float>(name, move(fn));
}

//===========================================================================
void Dim::perfGetValues (std::vector<PerfValue> & out, bool pretty) {
    auto & info = getInfo();
    bool mustSort = false;
    {
        shared_lock<shared_mutex> lk{info.mut};
        if (out.size() != info.counters.size()) {
            mustSort = true;
            out.clear();
            out.resize(info.counters.size());
            for (int i = 0; i < out.size(); ++i)
                out[i].pos = i;
        }
        for (unsigned i = 0; i < out.size(); ++i) {
            auto pos = out[i].pos;
            auto cnt = info.counters[pos].get();
            if (out[i].name.data() != cnt->name.data()) {
                out[i].name = cnt->name;
                out[i].value.clear();
            }
            if (out[i].value.empty()
                || out[i].raw != cnt->toDouble()
            ) {
                cnt->toString(out[i].value, pretty);
                out[i].raw = cnt->toDouble();
            }
        }
    }
    if (mustSort) {
        sort(
            out.begin(),
            out.end(),
            [](auto & a, auto & b){ return a.name < b.name; }
        );
    }
}
