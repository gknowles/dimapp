// Copyright Glen Knowles 2017 - 2021.
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

enum class PerfType {
    kInvalid,
    kInt,
    kUnsigned,
    kFloat,
    kNumTypes,
};

enum class PerfFormat {
    kMachine,   // Machine readable format
    kDefault,
    kSiUnits,
    kDuration,  // Value measured in seconds, whether float or int
};

struct PerfCounterBase {
    std::string name;
    PerfFormat format = PerfFormat::kDefault;

    virtual ~PerfCounterBase() = default;
    virtual PerfType type () const = 0;
    virtual void toString (std::string * out, bool pretty) const = 0;
    virtual double toDouble () const = 0;
};

template<typename T>
struct PerfCounter : PerfCounterBase, std::atomic<T> {
    using std::atomic<T>::operator=;

    PerfType type () const override;
    void toString (std::string * out, bool pretty) const override;
    double toDouble () const override;
};

template<typename T>
struct PerfFunc : PerfCounterBase {
    std::function<T()> fn;

    PerfType type () const override;
    void toString (std::string * out, bool pretty) const override;
    double toDouble () const override;
};


/****************************************************************************
*
*   Register performance counters
*
***/

PerfCounter<int> & iperf(
    std::string_view name, 
    PerfFormat fmt = PerfFormat::kDefault
);
PerfCounter<unsigned> & uperf(
    std::string_view name, 
    PerfFormat fmt = PerfFormat::kDefault
);
PerfCounter<float> & fperf(
    std::string_view name, 
    PerfFormat fmt = PerfFormat::kDefault
);

PerfFunc<int> & iperf(
    std::string_view name, 
    std::function<int()> fn, 
    PerfFormat fmt = PerfFormat::kDefault
);
PerfFunc<unsigned> & uperf(
    std::string_view name,
    std::function<unsigned()> fn, 
    PerfFormat fmt = PerfFormat::kDefault
);
PerfFunc<float> & fperf(
    std::string_view name, 
    std::function<float()> fn, 
    PerfFormat fmt = PerfFormat::kDefault
);


/****************************************************************************
*
*   Get counter values for display
*
***/

struct PerfValue {
    std::string_view name;
    std::string value;
    double raw;
    int pos;
    PerfType type;
};

// The pretty output localizes the numbers (e.g. commas) and is sorted.
//
// NOTE: If the passed in array is not empty it is assumed to still have the
//       contents returned by a previous call to this function.
void perfGetValues (std::vector<PerfValue> * out, bool pretty = false);

} // namespace
