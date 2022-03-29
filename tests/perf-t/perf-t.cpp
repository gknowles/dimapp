// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// perf-t.cpp - dim test perf
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#define EXPECT(...)                                                         \
    if (!bool(__VA_ARGS__)) {                                               \
        logMsgError() << "Line " << (line ? line : __LINE__) << ": EXPECT(" \
                      << #__VA_ARGS__ << ") failed";                        \
    }
#define EXPECT_VAL(cnt, name, val, expected) \
    testValue(line ? line : __LINE__, cnt, name, val, expected);


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static string_view getValue(string_view name) {
    static vector<PerfValue> values;
    perfGetValues(&values, true);
    auto i = ranges::find_if(values, [name](auto & a) {
        return a.name == name;
    });
    if (i == values.end())
        return {};
    return i->value;
}

//===========================================================================
template<typename T, typename U>
requires is_convertible_v<U, T>
static void testValue(
    int line,
    atomic<T> & cnt, 
    string_view name, 
    U val, 
    string_view expected
) {
    cnt = (T) val;
    auto pval = getValue(name);
    EXPECT(pval == expected);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    int line = 0;
    PerfValue val = {};

    //-----------------------------------------------------------------------
    // Integer counters
    auto & idef = iperf("test.int.default");
    auto & isi = iperf("test.int.siUnits", PerfFormat::kSiUnits);
    auto & idur = iperf("test.int.duration", PerfFormat::kDuration);
    auto & imc = iperf("test.int.machine", PerfFormat::kMachine);

    string name = "test.int.default";
    EXPECT_VAL(idef, name, 0, "0");
    EXPECT_VAL(idef, name, 1000000, "1,000,000");
    EXPECT_VAL(idef, name, -1, "-1");
    EXPECT_VAL(idef, name, -1000000, "-1,000,000");

    name = "test.int.siUnits";
    EXPECT_VAL(isi, name, 100, "100");
    EXPECT_VAL(isi, name, 1000, "1k");
    EXPECT_VAL(isi, name, 10'000, "10k");
    EXPECT_VAL(isi, name, 100'000, "100k");
    EXPECT_VAL(isi, name, 1'100'001, "1.1M");
    EXPECT_VAL(isi, name, 10'010'001, "10.01M");
    EXPECT_VAL(isi, name, 100'001'001, "100.001M");
    EXPECT_VAL(isi, name, -1000, "-1k");
    EXPECT_VAL(isi, name, -10000, "-10k");
    EXPECT_VAL(isi, name, -100000, "-100k");

    name = "test.int.duration";
    EXPECT_VAL(idur, name, 150, "2m 30s");
    EXPECT_VAL(idur, name, -150, "-2m 30s");

    name = "test.int.machine";
    EXPECT_VAL(imc, name, 1'000'000, "1000000");
    EXPECT_VAL(imc, name, -100, "-100");

    //-----------------------------------------------------------------------
    // Unsigned counters
    auto & udef = uperf("test.unsigned.default");
    auto & usi = uperf("test.unsigned.siUnits", PerfFormat::kSiUnits);
    auto & udur = uperf("test.unsigned.duration", PerfFormat::kDuration);
    auto & umc = uperf("test.unsigned.machine", PerfFormat::kMachine);

    name = "test.unsigned.default";
    EXPECT_VAL(udef, name, 1'100'001, "1,100,001");
    EXPECT_VAL(udef, name, -1, "4,294,967,295");

    name = "test.unsigned.siUnits";
    EXPECT_VAL(usi, name, 1'100'001, "1.1M");
    EXPECT_VAL(usi, name, -1, "4.294G");

    name = "test.unsigned.duration";
    EXPECT_VAL(udur, name, 150, "2m 30s");
    EXPECT_VAL(udur, name, -150, "136y 5w");

    name = "test.unsigned.machine";
    EXPECT_VAL(umc, name, 1'000'000, "1000000");
    EXPECT_VAL(umc, name, -1, "4294967295");

    //-----------------------------------------------------------------------
    // Floating point counters
    auto & fdef = fperf("test.float.default");
    auto & fsi = fperf("test.float.siUnits", PerfFormat::kSiUnits);
    auto & fdur = fperf("test.float.duration", PerfFormat::kDuration);
    auto & fmc = fperf("test.float.machine", PerfFormat::kMachine);

    name = "test.float.default";
    EXPECT_VAL(fdef, name, 1'000'000, "1e+06");
    EXPECT_VAL(fdef, name, -1000, "-1,000");
    EXPECT_VAL(fdef, name, 1234.5678, "1,234.567");
    EXPECT_VAL(fdef, name, 1.23, "1.23");
    EXPECT_VAL(fdef, name, 1.234, "1.234");

    name = "test.float.siUnits";
    EXPECT_VAL(fsi, name, 1'000'000, "1M");
    EXPECT_VAL(fsi, name, -100.23, "-100.23");
    EXPECT_VAL(fsi, name, 1.25e-8, "12.5n");

    name = "test.float.duration";
    EXPECT_VAL(fdur, name, 1'000'000, "1w 5d");
    EXPECT_VAL(fdur, name, 1.5, "1s 500ms");
    EXPECT_VAL(fdur, name, -0.5, "-500ms");

    name = "test.float.machine";
    EXPECT_VAL(fmc, name, 1'000'000, "1e+06");

    testSignalShutdown();
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv);
}
