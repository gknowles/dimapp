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
    if (!pretty) {
        StrFrom<T> str{val};
        out = str;
        return;
    }

    ostringstream os;
    os.imbue(locale(""));
    os << val;
    out = os.str();
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
inline float PerfCounter<T>::toFloat () const {
    return (float) *this;
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
inline float PerfFunc<T>::toFloat () const {
    return (float) fn();
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
    {
        shared_lock<shared_mutex> lk{info.mut};
        out.resize(info.counters.size());
        for (unsigned i = 0; i < out.size(); ++i) {
            out[i].name = info.counters[i]->name;
            info.counters[i]->toString(out[i].value, pretty);
        }
    }
    if (pretty) {
        sort(
            out.begin(), 
            out.end(),
            [](auto & a, auto & b){ return a.name < b.name; }
        );
    }
}
