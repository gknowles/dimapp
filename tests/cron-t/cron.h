// Copyright Glen Knowles 2018 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// References:
//  https://en.wikipedia.org/wiki/Cron
//  https://www.freeformatter.com/cron-expression-generator-quartz.html
//  http://man7.org/linux/man-pages/man5/crontab.5.html (vixie)
//  https://crontab.guru/ (vixie)
//
// cron.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "basic/str.h"
#include "basic/tokentable.h"
#include "basic/types.h"
#include "core/time.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

class Cron {
public:
    enum WithFlags : int {
        WithDefault             = 0,
        WithSeconds             = 1 << 0,
        WithYears               = 1 << 1,
        WithLastDom             = 1 << 2,
        WithLastDomOffset       = 1 << 3,
        WithWeekDays            = 1 << 4,
        WithMultipleWeekDays    = 1 << 5,
        WithLastDow             = 1 << 6,
        WithNthDow              = 1 << 7,
        WithSoftStarForDays     = 1 << 8, // dow/dom of '*' ignored if the
                                          //   other is not just a '*'
        WithQuestionForDays     = 1 << 9, // either dow or dom must be '?'

        WithSlash               = 1 << 10,
        WithSlashAutoRange      = 1 << 11, // n/i -> n-<max value>/i
        WithNames               = 1 << 12,
        WithNamesInRanges       = 1 << 13,
        WithMacros              = 1 << 14, // @monthly, @daily, etc
    };

    static constexpr uint8_t maskBit(size_t pos);
    static constexpr bool getBit(const uint8_t bits[], size_t pos);
    static constexpr void setBit(uint8_t bits[], size_t pos, bool value = true);

protected:
    enum Macro {
        kInvalid,
        kYearly,
        kMonthly,
        kWeekly,
        kDaily,
        kHourly,
    };

    static std::vector<TokenTable::Token> const s_macros;
    static const TokenTable s_macroTbl;
    static const TokenTable s_wdayTbl;
    static const TokenTable s_monTbl;
};

template<int Flags = Cron::WithDefault>
class CronDef : public Cron {
public:
    static constexpr int s_flags = Flags;

public:
    bool parse(std::string_view src);

    std::string str() const;

    bool contains(Dim::TimePoint time) const;
    Dim::TimePoint lowerBound(Dim::TimePoint time) const;
    Dim::TimePoint upperBound(Dim::TimePoint time) const;

private:
    template<int Flags>
    friend std::ostream & operator<<(
        std::ostream & os,
        Dim::CronDef<Flags> & val
    ) {
        return os << val.str();
    }

private:
    enum {
        kSecPos = 0,
        kMinPos = kSecPos + ((Flags & WithSeconds) ? 60 : 0),
        kHourPos = kMinPos + 60,
        kDomPos = kHourPos + 24,
        kLastDomPos = kDomPos + 31,
        kWeekDomPos = kLastDomPos + ((Flags & WithLastDomOffset)
            ? 30 : (Flags & WithLastDom) ? 1 : 0),
        kLastAndWeekDomPos = kWeekDomPos + ((Flags & WithWeekDays) ? 31 : 0),
        kHasDomPos = kLastAndWeekDomPos + ((~Flags & WithWeekDays) ? 0
            : (Flags & WithLastDomOffset) ? 30 : (Flags & WithLastDom) ? 1 : 0),
        kMonPos = kHasDomPos + 1,
        kDowPos = kMonPos + 12,
        kLastDowPos = kDowPos + 7,
        kNthDowPos = kLastDowPos + ((Flags & WithLastDow) ? 7 : 0),
        kYearPos = kNthDowPos + ((Flags & WithNthDow) ? 35 : 0),
        kBits = kYearPos + ((Flags & WithYears) ? 130 : 0),
    };
    using Data = uint8_t[(kBits + 7) / 8];

    bool parseField(
        Data * data,
        unsigned bitpos,
        std::string_view * src,
        int minLow,
        int maxHigh,
        const Dim::TokenTable * tbl = nullptr
    );

    Data m_data;
};

using StdCron = CronDef<Cron::WithSoftStarForDays>;
using VixieCron = CronDef<Cron::WithSoftStarForDays | Cron::WithSlash
    | Cron::WithNames | Cron::WithMacros>;
using QuartzCron = CronDef<Cron::WithSeconds | Cron::WithYears
    | Cron::WithQuestionForDays
    | Cron::WithLastDomOffset
    | Cron::WithMultipleWeekDays
    | Cron::WithLastDow | Cron::WithNthDow
    | Cron::WithSlashAutoRange
    | Cron::WithNamesInRanges
    | Cron::WithMacros>;
using AnyCron = CronDef<Cron::WithSoftStarForDays
    | Cron::WithLastDomOffset
    | Cron::WithMultipleWeekDays
    | Cron::WithLastDow | Cron::WithNthDow
    | Cron::WithSlashAutoRange
    | Cron::WithNamesInRanges
    | Cron::WithMacros>;

} // namespace


/****************************************************************************
*
*   Cron
*
***/

//===========================================================================
constexpr uint8_t Dim::Cron::maskBit(size_t pos) {
    return 1 << (8 - pos % 8 - 1);
}

//===========================================================================
constexpr bool Dim::Cron::getBit(const uint8_t bits[], size_t pos) {
    return bits[pos / 8] & maskBit(pos);
}

//===========================================================================
constexpr void Dim::Cron::setBit(uint8_t bits[], size_t pos, bool value) {
    if (value) {
        bits[pos / 8] |= maskBit(pos);
    } else {
        bits[pos / 8] &= ~maskBit(pos);
    }
}


/****************************************************************************
*
*   CronDef
*
***/

//===========================================================================
template<int Flags>
bool Dim::CronDef<Flags>::parse(std::string_view src) {
    Data d = {};
    src = trim(src);
    if (src.empty())
        return false;
    if (src[0] == '@') {
        auto withSY = Flags & (WithSeconds | WithYears);
        auto macro = s_macroTbl.find(src.substr(1), kInvalid);
        if (macro == kInvalid)
            return false;
        if (withSY == 0) {
            switch (macro) {
            case kInvalid: break;
            case kYearly: src = "0 0 1 1 *"; break;
            case kMonthly: src = "0 0 1 * *"; break;
            case kWeekly: src = "0 0 * * 0"; break;
            case kDaily: src = "0 0 * * *"; break;
            case kHourly: src = "0 * * * *"; break;
            }
        } else if (withSY == WithSeconds) {
            switch (macro) {
            case kInvalid: break;
            case kYearly: src = "0 0 0 1 1 *"; break;
            case kMonthly: src = "0 0 0 1 * *"; break;
            case kWeekly: src = "0 0 0 * * 0"; break;
            case kDaily: src = "0 0 0 * * *"; break;
            case kHourly: src = "0 0 * * * *"; break;
            }
        } else if (withSY == WithYears) {
            switch (macro) {
            case kInvalid: break;
            case kYearly: src = "0 0 1 1 * *"; break;
            case kMonthly: src = "0 0 1 * * *"; break;
            case kWeekly: src = "0 0 * * 0 *"; break;
            case kDaily: src = "0 0 * * * *"; break;
            case kHourly: src = "0 * * * * *"; break;
            }
        } else if (withSY == (WithSeconds | WithYears)) {
            switch (macro) {
            case kInvalid: break;
            case kYearly: src = "0 0 0 1 1 * *"; break;
            case kMonthly: src = "0 0 0 1 * * *"; break;
            case kWeekly: src = "0 0 0 * * 0 *"; break;
            case kDaily: src = "0 0 0 * * * *"; break;
            case kHourly: src = "0 0 * * * * *"; break;
            }
        }
        assert(src[0] != '@');
    }

    if constexpr (Flags & WithSeconds) {
        if (!parseField(&d, kSecPos, &src, 0, 59, nullptr))
            return false;
    }
    if (!parseField(&d, kMinPos, &src, 0, 59, nullptr)
        || !parseField(&d, kHourPos, &src, 0, 23, nullptr)
        || !parseField(&d, kDomPos, &src, 1, 31, nullptr)
        || !parseField(&d, kMonPos, &src, 1, 12, &s_monTbl)
        || !parseField(&d, kDowPos, &src, 0, 7, &s_wdayTbl)
    ) {
        return false;
    }
    if constexpr (Flags & WithYears) {
        if (!parseField(&d, kYearPos, &src, 1970, 2099, nullptr))
            return false;
    }

    memcpy(&m_data, &d, sizeof m_data);
    return true;
}

//===========================================================================
template<int Flags>
bool Dim::CronDef<Flags>::contains(Dim::TimePoint time) const {
    return false;
}

//===========================================================================
template<int Flags>
Dim::TimePoint Dim::CronDef<Flags>::lowerBound(Dim::TimePoint time) const {
    return {};
}

//===========================================================================
template<int Flags>
Dim::TimePoint Dim::CronDef<Flags>::upperBound(Dim::TimePoint time) const {
    return {};
}
