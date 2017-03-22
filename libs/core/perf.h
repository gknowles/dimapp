// perf.h - dim core
#pragma once

#include "config/config.h"

#include <atomic>
#include <functional>
#include <string_view>

namespace Dim {

struct PerfCounterBase {
    std::string name;
    virtual void toString (std::string & out) const = 0;
    virtual float toFloat () const = 0;
};

template<typename T>
struct PerfCounter : PerfCounterBase, std::atomic<T> {
    void toString (std::string & out) const override;
    float toFloat () const override;
};

template<typename T>
struct PerfFunc : PerfCounterBase {
    std::function<T()> fn;
    void toString (std::string & out) const override;
    float toFloat () const override;
};


PerfCounter<int> & perfInt(std::string_view name);
PerfCounter<unsigned> & perfUnsigned(std::string_view name);
PerfCounter<float> & perfFloat(std::string_view name);

PerfFunc<int> & perfInt(std::string_view name, std::function<int()> fn);
PerfFunc<unsigned> & perfUnsigned(
    std::string_view name,
    std::function<unsigned()> fn
);
PerfFunc<float> & perfFloat(std::string_view name, std::function<float()> fn);

struct PerfValue {
    std::string_view name;
    std::string value;
};
void perfGetValues (std::vector<PerfValue> & out);

} // namespace
