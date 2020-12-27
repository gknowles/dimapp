// Copyright Glen Knowles 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// crontest.cpp - dim test cron
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


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
template<typename Cron>
static void parseTest(
    Cron * cron,
    string_view in,
    string_view out,
    bool fail = false
) {
    if (!fail != cron->parse(in)) {
        logMsgError() << "(" << in << ").parse >> "
            << (!fail ? "false" : "true");
    } else if (out != cron->str()) {
        logMsgError() << in << " >> " << cron->str() << ", expected " << out;
    }
}

//===========================================================================
static void writeField(
    string * out,
    uint8_t const * data,
    unsigned fpos,
    unsigned minLow,
    unsigned maxHigh
) {
    vector<pair<unsigned, unsigned>> ranges;
    for (auto i = minLow; !Cron::getBit(data, fpos + i - minLow); ++i) {
        if (i > maxHigh)
            goto COMPACT;
    }

COMPACT:
    ;
}

//===========================================================================
template<Cron::WithFlags Flags>
std::string Dim::CronDef<Flags>::str() const {
    std::string out;
    if (Flags & WithSeconds)
        writeField(&out, m_data, kSecPos, 0, 59);
    writeField(&out, m_data, kMinPos, 0, 59);
    writeField(&out, m_data, kHourPos, 0, 23);
    // Day of month
    writeField(&out, m_data, kMonPos, 1, 12);
    // Day of week
    if (Flags & WithYears)
        writeField(&out, m_data, kYearPos, 1970, 2099);
    return out;
}

//===========================================================================
// *
// <low>-<high>/<interval>[{#[1-5] | L | W}], ...
// also, if day of month: L[-<offset>][W], ...
template<Cron::WithFlags Flags>
bool Dim::CronDef<Flags>::parseField(
    Dim::CronDef<Flags>::Data * data,
    unsigned fpos,
    std::string_view * src,
    int minLow,
    int maxHigh,
    const Dim::TokenTable * tbl
) {
    while (src->front() == ' ')
        src->remove_prefix(1);

    char * eptr;
    bool hasValue = false;
    for (;;) {
        int low = 0;
        int high = 0;
        int interval = 1;
        int nth = 0;
        bool last = false;
        int offset = 0;
        bool week = false;

        bool hasRange = false;
        bool isStar = false;

        low = strToUint(*src, &eptr);
        if (eptr == src->data()) {
            if (src->front() == '*') {
                src->remove_prefix(1);
                low = minLow;
                high = maxHigh;
                isStar = true;
                goto PARSE_INTERVAL;
            } else if (tbl) {
                if (!tbl->find(&low, src->substr(0, 3)))
                    return false;
                src->remove_prefix(3);
            } else if (fpos == kDomPos
                && (src->front() == 'l' || src->front() == 'L')
                && (Flags & WithLastDom)
            ) {
                src->remove_prefix(1);
                if (src->front() == '-') {
                    if (~Flags & WithLastDomOffset)
                        return false;
                    src->remove_prefix(1);
                    offset = strToUint(*src, &eptr);
                    if (eptr == src->data())
                        return false;
                    src->remove_prefix(eptr - src->data());
                }
                if (src->front() == 'w' || src->front() == 'W') {
                    if (~Flags & WithWeekDays)
                        return false;
                    week = true;
                    src->remove_prefix(1);
                }
            } else {
                return false;
            }
            goto RECORD_RANGE;
        }
        src->remove_prefix(eptr - src->data());
        if (low < minLow)
            return false;
        if (src->front() == '-') {
            hasRange = true;
            high = strToUint(*src, &eptr);
            if (eptr == src->data()) {
                if (!tbl || !tbl->find(&high, src->substr(0, 3)))
                    return false;
                src->remove_prefix(3);
            } else {
                src->remove_prefix(eptr - src->data());
            }
            if (high < low)
                return false;
        } else {
            high = low;
        }
        if (high > maxHigh)
            return false;

    PARSE_INTERVAL:
        if (src->front() == '/') {
            if (~Flags & WithSlash)
                return false;
            isStar = false;
            interval = strToUint(*src, &eptr);
            if (eptr == src->data())
                return false;
            if (!hasRange) {
                if (~Flags & WithSlashAutoRange)
                    return false;
                high = maxHigh;
            }
            src->remove_prefix(eptr - src->data());
        }
        switch (src->front()) {
        case '#':
            isStar = false;
            if (fpos != kDowPos)
                return false;
            nth = strToUint(*src, &eptr);
            if (eptr == src->data())
                return false;
            if (nth < 1 || nth > 5)
                return false;
            src->remove_prefix(eptr - src->data());
            break;
        case 'l': case 'L':
            isStar = false;
            if (fpos != kDowPos)
                return false;
            last = true;
            src->remove_prefix(1);
            break;
        case 'w': case 'W':
            isStar = false;
            if (fpos != kDomPos)
                return false;
            week = true;
            src->remove_prefix(1);
            break;
        }

    RECORD_RANGE:
        switch (src->front()) {
        case 0:
        case ' ':
        case ',':
            break;
        default:
            return false;
        }

        if (!isStar)
            hasValue = true;

        for (int i = low; i <= high; i += interval) {
            if (fpos == kSecPos && (Flags & WithSeconds)
                || fpos == kMinPos
                || fpos == kHourPos
                || fpos == kMonPos
                || fpos == kYearPos && (Flags & WithYears)
            ) {
                setBit(*data, fpos + i - minLow);
            } else if (fpos == kDomPos) {
                if (last) {
                    assert(Flags & WithLastDom);
                    assert(!offset || (Flags & WithLastDomOffset));
                    if (week) {
                        assert(Flags & WithWeekDays);
                        setBit(*data, kLastAndWeekDomPos + offset);
                    } else {
                        setBit(*data, kLastDomPos + offset);
                    }
                } else {
                    if (week) {
                        assert(Flags & WithWeekDays);
                        setBit(*data, kWeekDomPos + i);
                    } else {
                        setBit(*data, kDomPos + i);
                    }
                }
            } else if (fpos == kDowPos) {
                if (nth) {
                    setBit(*data, kNthDowPos + 7 * (nth - 1) + i % 7);
                } else if (last) {
                    setBit(*data, kLastDowPos + i % 7);
                } else {
                    setBit(*data, kDowPos + i % 7);
                }
            } else {
                assert(!"Internal dimcron error");
            }
        }

        if (src->front() != ',')
            break;

        src->remove_prefix(1);
    }

    if (fpos == kDomPos) {
        if (hasValue)
            setBit(*data, kHasDomPos);
    }
    if (fpos == kDowPos) {
        if (Flags & WithSoftStarForDays) {
            if (!hasValue) {
                for (int i = 0; i <= 6; ++i)
                    setBit(*data, kDowPos + i, false);
            } else if (!getBit(*data, kHasDomPos)) {
                for (int i = 1; i <= 31; ++i)
                    setBit(*data, kDomPos + i, false);
            }
        }
    }
    return true;
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    // int line = 0;

    AnyCron cron;
    parseTest(&cron, "@daily", "0 0 * * *");

    if (int errs = logGetMsgCount(kLogTypeError)) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** TEST FAILURES: " << errs << endl;
        appSignalShutdown(EX_SOFTWARE);
    } else {
        cout << "All tests passed" << endl;
        appSignalShutdown(EX_OK);
    }
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv, fAppClient);
}
