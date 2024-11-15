// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// perf.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <atomic>
#include <functional>
#include <string>
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
    kDefault,
    kDuration,  // Value measured in seconds, whether float or int
    kMachine,   // Machine readable format
    kSiUnits,
};


/****************************************************************************
*
*   Register performance counters
*
***/

std::atomic<int> & iperf(
    std::string_view name,
    PerfFormat fmt = PerfFormat::kDefault
);
std::atomic<unsigned> & uperf(
    std::string_view name,
    PerfFormat fmt = PerfFormat::kDefault
);
std::atomic<float> & fperf(
    std::string_view name,
    PerfFormat fmt = PerfFormat::kDefault
);

std::function<int()> & iperf(
    std::string_view name,
    std::function<int()> fn,
    PerfFormat fmt = PerfFormat::kDefault
);
std::function<unsigned()> & uperf(
    std::string_view name,
    std::function<unsigned()> fn,
    PerfFormat fmt = PerfFormat::kDefault
);
std::function<float()> & fperf(
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
    PerfFormat format;
};

// The pretty output localizes the numbers (e.g. commas) and is sorted.
//
// NOTE: If the passed in array is not empty it is assumed to have the contents
//       returned by a previous call to this function.
void perfGetValues(std::vector<PerfValue> * out, bool pretty = false);

} // namespace
