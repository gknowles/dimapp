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
    mutex mut;
    vector<unique_ptr<PerfCounterBase>> counters;
};
} // namespace


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
    lock_guard<mutex> lk{info.mut};
    info.counters.push_back(make_unique<PerfCounter<T>>());
    auto & cnt = static_cast<PerfCounter<T>&>(*info.counters.back());
    cnt.name = name;
    return cnt;
}

//===========================================================================
template <typename T>
static PerfFunc<T> & perf(string_view name, function<T()> fn) {
    auto & info = getInfo();
    lock_guard<mutex> lk{info.mut};
    info.counters.push_back(make_unique<PerfFunc<T>>());
    auto & cnt = static_cast<PerfFunc<T>&>(*info.counters.back());
    cnt.name = name;
    cnt.fn = fn;
    return cnt;
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
inline void PerfCounter<T>::toString (std::string & out) const {
    out = ::to_string(*this);
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
inline void PerfFunc<T>::toString (std::string & out) const {
    out = ::to_string(fn());
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
PerfCounter<int> & Dim::perfInt(string_view name) {
    return perf<int>(name);
}

//===========================================================================
PerfCounter<unsigned> & Dim::perfUnsigned(string_view name) {
    return perf<unsigned>(name);
}

//===========================================================================
PerfCounter<float> & Dim::perfFloat(string_view name) {
    return perf<float>(name);
}

//===========================================================================
PerfFunc<int> & Dim::perfInt(string_view name, function<int()> fn) {
    return perf<int>(name, fn);
}

//===========================================================================
PerfFunc<unsigned> & Dim::perfUnsigned(
    string_view name, 
    function<unsigned()> fn
) {
    return perf<unsigned>(name, fn);
}

//===========================================================================
PerfFunc<float> & Dim::perfFloat(string_view name, function<float()> fn) {
    return perf<float>(name, fn);
}

//===========================================================================
void Dim::perfGetValues (std::vector<PerfValue> & out) {
    auto & info = getInfo();
    lock_guard<mutex> lk{info.mut};
    out.resize(info.counters.size());
    for (int i = 0; i < out.size(); ++i) {
        out[i].name = info.counters[i]->name;
        info.counters[i]->toString(out[i].value);
    }
}
