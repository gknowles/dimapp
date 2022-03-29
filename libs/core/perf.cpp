// Copyright Glen Knowles 2017 - 2022.
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

struct PerfBase {
    virtual ~PerfBase() = default;

    virtual PerfType type() const = 0;
    virtual PerfFormat format() const = 0;
    virtual double toDouble() const = 0;
    virtual void toString(string * out, bool pretty) const = 0;

    string m_name;
};

template<typename T, PerfFormat Fmt>
struct PerfAtomic final : PerfBase {
    PerfType type() const override;
    PerfFormat format() const override { return Fmt; }
    double toDouble() const override;
    void toString(string * out, bool pretty) const override;

    atomic<T> m_val;
};

template<typename T, PerfFormat Fmt>
struct PerfFunc final : PerfBase {
    PerfType type() const override;
    PerfFormat format() const override { return Fmt; }
    double toDouble() const override;
    void toString(string * out, bool pretty) const override;

    function<T()> m_fn;
};

struct PerfInfo {
    // mutex only needed for the vector itself.
    shared_mutex mut;
    vector<unique_ptr<PerfBase>> counters;
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
template <typename T, PerfFormat Fmt>
static atomic<T> & perf(string_view name) {
    auto & info = getInfo();
    unique_lock lk{info.mut};
    info.counters.push_back(make_unique<PerfAtomic<T, Fmt>>());
    auto & cnt = static_cast<PerfAtomic<T, Fmt> &>(*info.counters.back());
    cnt.m_name = name;
    return cnt.m_val;
}

//===========================================================================
template <typename T, PerfFormat Fmt>
static function<T()> & perf(string_view name, function<T()> && fn) {
    auto & info = getInfo();
    unique_lock lk{info.mut};
    info.counters.push_back(make_unique<PerfFunc<T, Fmt>>());
    auto & cnt = static_cast<PerfFunc<T, Fmt> &>(*info.counters.back());
    cnt.m_name = name;
    cnt.m_fn = move(fn);
    return cnt.m_fn;
}

//===========================================================================
template <typename T>
static PerfType perfType() {
    if constexpr (is_same_v<T, int>) {
        return PerfType::kInt;
    } else if constexpr (is_same_v<T, unsigned>) {
        return PerfType::kUnsigned;
    } else if constexpr (is_same_v<T, float>) {
        return PerfType::kFloat;
    } else {
        static_assert(!is_same_v<T, T>, "unknown perf counter type");
        return PerfType::kInvalid;
    }
}


/****************************************************************************
*
*   PerfPrint
*
***/

//===========================================================================
template<typename T, PerfFormat Fmt>
struct PerfPrint {
    void format(string * out, T val) const;
};

//===========================================================================
static void appendWithCommas(string * out, string_view str) {
    auto num = str.size();
    auto ptr = str.data();
    assert(num && *ptr);
    assert(*ptr != '-');
    auto eptr = ptr + num;
    auto ecomma = eptr;
    num = (num - 1) % 3 + 1;
    auto olen = out->size();
    out->resize(olen + str.size() + (str.size() - num) / 3);
    auto optr = out->data() + olen;
    switch (num) {
        case 3: *optr++ = *ptr++; [[fallthrough]];
        case 2: *optr++ = *ptr++; [[fallthrough]];
        case 1: *optr++ = *ptr++;
    }
    while (ptr != ecomma) {
        *optr++ = ',';
        *optr++ = *ptr++;
        *optr++ = *ptr++;
        *optr++ = *ptr++;
    }
}

//===========================================================================
template <typename T>
requires is_integral_v<T>
struct PerfPrint<T, PerfFormat::kDefault> {
    void format(string * out, T val) const {
        out->clear();
        if constexpr (is_signed_v<T>) {
            if (val < 0) {
                out->assign(1, '-');
                val = -val;
            }
        }
        auto str = StrFrom{val};
        appendWithCommas(out, str.view());
    }
};

//===========================================================================
template <>
struct PerfPrint<float, PerfFormat::kDefault> {
    void format(string * out, float val) {
        if (val < 0) {
            out->assign(1, '-');
            val = -val;
        } else {
            out->clear();
        }
        auto str = StrFrom{val};
        auto v = str.view();
        if (v.find('e') != string_view::npos) {
            out->append(v);
            return;
        }
        auto decimal = v.find('.'); 
        if (decimal == string_view::npos) {
            appendWithCommas(out, v);
        } else {
            appendWithCommas(out, v.substr(0, decimal));
            v = v.substr(decimal, 4);
            out->append(v);
        }
    }
};

//===========================================================================
template<typename T>
struct PerfPrint<T, PerfFormat::kDuration> {
    void format(string * out, T val) const {
        Duration dur;
        if constexpr (is_same_v<T, float>) {
            auto tmp = chrono::duration<double>(val);
            dur = duration_cast<Duration>(tmp);
        } else {
            auto tmp = chrono::seconds(val);
            dur = duration_cast<Duration>(tmp);
        }
        *out = toString(dur, DurationFormat::kTwoPart);
    }
};

//===========================================================================
template<typename T>
struct PerfPrint<T, PerfFormat::kMachine> {
    void format(string * out, T val) const {
        auto str = StrFrom{val};
        *out = str.view();
    }
};

//===========================================================================
template<typename T>
requires is_integral_v<T>
struct PerfPrint<T, PerfFormat::kSiUnits> {
    void format(string * out, T val) const {
        out->clear();
        if constexpr (is_signed_v<T>) {
            if (val < 0) {
                out->push_back('-');
                val = -val;
            }
        }
        auto suffix = 0;
        auto decimal = 0;
        while (val >= 1000) {
            suffix += 1;
            decimal = val % 1000;
            val /= 1000;
        }

        auto str = StrFrom{val};
        out->append(str.view());

        if (decimal) {
            // Include decimal point and up to three more digits
            out->push_back('.');
            for (;;) {
                out->push_back('0' + (unsigned char) (decimal / 100));
                decimal %= 100;
                if (!decimal)
                    break;
                decimal *= 10;
            }
        }
        if (suffix) 
            out->push_back(" kMGTPEZY"[suffix]);
    }
};

//===========================================================================
template<>
struct PerfPrint<float, PerfFormat::kSiUnits> {
    void format(string * out, float val) const {
        if (val < 0) {
            out->assign(1, '-');
            val = -val;
        } else {
            out->clear();
        }
        auto suffix = 0;
        while (val >= 1000) {
            suffix += 1;
            val /= 1000;
        }
        while (val && val < 1) {
            suffix -= 1;
            val *= 1000;
        }

        auto str = StrFrom{val};
        auto v = str.view();
        if (auto pos = v.find('.'); pos != string_view::npos) {
            auto len = min(v.size(), pos + 4);
            v = v.substr(0, len);
        }
        out->append(v);
        if (suffix) 
            out->push_back("yzafpnum kMGTPEZY"[suffix + 8]);
    }
};


/****************************************************************************
*
*   PerfAtomic
*
***/

//===========================================================================
template<typename T, PerfFormat Fmt>
inline double PerfAtomic<T, Fmt>::toDouble() const {
    return (double) m_val;
}

//===========================================================================
template<typename T, PerfFormat Fmt>
inline void PerfAtomic<T, Fmt>::toString(string * out, bool pretty) const {
    if (pretty) {
        PerfPrint<T, Fmt>().format(out, m_val);
    } else {
        PerfPrint<T, PerfFormat::kMachine>().format(out, m_val);
    }
}

//===========================================================================
template<typename T, PerfFormat Fmt>
inline PerfType PerfAtomic<T, Fmt>::type() const {
    return perfType<T>();
}


/****************************************************************************
*
*   PerfFunc
*
***/

//===========================================================================
template<typename T, PerfFormat Fmt>
inline double PerfFunc<T, Fmt>::toDouble () const {
    return (double) m_fn();
}

//===========================================================================
template<typename T, PerfFormat Fmt>
inline void PerfFunc<T, Fmt>::toString (string * out, bool pretty) const {
    if (pretty) {
        PerfPrint<T, Fmt>().format(out, m_fn());
    } else {
        PerfPrint<T, PerfFormat::kMachine>().format(out, m_fn());
    }
}

//===========================================================================
template<typename T, PerfFormat Fmt>
inline PerfType PerfFunc<T, Fmt>::type () const {
    return perfType<T>();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iPerfInitialize() {
    auto & info = getInfo();
    unique_lock lk{info.mut};
    s_numStatic = info.counters.size();
}

//===========================================================================
void Dim::iPerfDestroy() {
    auto & info = getInfo();
    unique_lock lk{info.mut};
    assert(info.counters.size() >= s_numStatic);
    info.counters.resize(s_numStatic);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
template<typename T>
static atomic<T> & perf(string_view name, PerfFormat fmt) {
    using enum PerfFormat;
    switch (fmt) {
    default:
        assert(!"Unknown performance counter display format");
        [[fallthrough]];
    case kDefault:  return perf<T, kDefault>(name);
    case kSiUnits:  return perf<T, kSiUnits>(name);
    case kDuration: return perf<T, kDuration>(name);
    case kMachine:  return perf<T, kMachine>(name);
    }
}

//===========================================================================
atomic<int> & Dim::iperf(string_view name, PerfFormat fmt) {
    return perf<int>(name, fmt);
}

//===========================================================================
atomic<unsigned> & Dim::uperf(string_view name, PerfFormat fmt) {
    return perf<unsigned>(name, fmt);
}

//===========================================================================
atomic<float> & Dim::fperf(string_view name, PerfFormat fmt) {
    return perf<float>(name, fmt);
}

//===========================================================================
template<typename T>
static function<T()> & perf(
    string_view name, 
    function<T()> && fn, 
    PerfFormat fmt
) {
    using enum PerfFormat;
    switch (fmt) {
    default:
        assert(!"Unknown performance counter display format");
        [[fallthrough]];
    case kDefault:  return perf<T, kDefault>(name, move(fn));
    case kSiUnits:  return perf<T, kSiUnits>(name, move(fn));
    case kDuration: return perf<T, kDuration>(name, move(fn));
    case kMachine:  return perf<T, kMachine>(name, move(fn));
    }
}

//===========================================================================
function<int()> & Dim::iperf(
    string_view name, 
    function<int()> fn, 
    PerfFormat fmt
) {
    return perf<int>(name, move(fn), fmt);
}

//===========================================================================
function<unsigned()> & Dim::uperf(
    string_view name,
    function<unsigned()> fn, 
    PerfFormat fmt
) {
    return perf<unsigned>(name, move(fn), fmt);
}

//===========================================================================
function<float()> & Dim::fperf(
    string_view name, 
    function<float()> fn, 
    PerfFormat fmt
) {
    return perf<float>(name, move(fn), fmt);
}

//===========================================================================
void Dim::perfGetValues (vector<PerfValue> * outptr, bool pretty) {
    auto & out = *outptr;
    auto & info = getInfo();
    bool mustSort = false;
    {
        shared_lock lk{info.mut};
        if (out.size() != info.counters.size()) {
            mustSort = true;
            out.clear();
            out.resize(info.counters.size());
            for (int i = 0; i < out.size(); ++i)
                out[i].pos = i;
        }
        for (unsigned i = 0; i < out.size(); ++i) {
            auto & oval = out[i];
            auto cnt = info.counters[oval.pos].get();
            if (oval.name.data() != cnt->m_name.data()) {
                oval.name = cnt->m_name;
                oval.value.clear();
            }
            auto raw = cnt->toDouble();
            if (oval.value.empty() || oval.raw != raw) {
                cnt->toString(&oval.value, pretty);
                oval.raw = raw;
                oval.type = cnt->type();
                oval.format = cnt->format();
            }
        }
    }
    if (mustSort) {
        ranges::sort(
            out,
            [](auto & a, auto & b){ return a.name < b.name; }
        );
    }
}
