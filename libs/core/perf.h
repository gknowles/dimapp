// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// perf.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <atomic>
#include <functional>
#include <string_view>

namespace Dim {

/****************************************************************************
*
*   Declarations
*
***/

struct PerfCounterBase {
    std::string name;

    virtual ~PerfCounterBase() = default;
    virtual void toString (std::string & out, bool pretty) const = 0;
    virtual float toFloat () const = 0;
};

template<typename T>
struct PerfCounter : PerfCounterBase, std::atomic<T> {
    void toString (std::string & out, bool pretty) const override;
    float toFloat () const override;
};

template<typename T>
struct PerfFunc : PerfCounterBase {
    std::function<T()> fn;
    void toString (std::string & out, bool pretty) const override;
    float toFloat () const override;
};


/****************************************************************************
*
*   Register performance counters
*
***/

PerfCounter<int> & iperf(std::string_view name);
PerfCounter<unsigned> & uperf(std::string_view name);
PerfCounter<float> & fperf(std::string_view name);

PerfFunc<int> & iperf(std::string_view name, std::function<int()> fn);
PerfFunc<unsigned> & uperf(
    std::string_view name,
    std::function<unsigned()> fn
);
PerfFunc<float> & fperf(std::string_view name, std::function<float()> fn);


/****************************************************************************
*
*   Get counter values for display
*
***/

struct PerfValue {
    std::string_view name;
    std::string value;
};

// The pretty output has localizes the numbers (e.g. commas) and is sorted.
void perfGetValues (std::vector<PerfValue> & out, bool pretty = false);

} // namespace
