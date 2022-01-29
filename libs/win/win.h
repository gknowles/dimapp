// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// win.h - dim windows platform
#pragma once

#include "cppconf/cppconf.h"

#include "winsvcctrl.h"

#include <vector>

namespace Dim {


/****************************************************************************
*
*   Registry
*
***/

class WinRegTree {
public:
    enum class Registry {
        kClassesRoot,
        kCurrentConfig,
        kCurrentUser,
        kLocalMachine,
        kPerformanceData,
        kUsers,
    };
    enum class ValueType {
        kBinary,
        kDword,
        kDwordLittleEndian,
        kDwordBigEndian,
        kExpandSz,
        kLink,
        kMultiSz,
        kNone,
        kQword,
        kQwordLittleEndian,
        kSz,
    };
    struct Value {
        std::string name;
        ValueType type;
        std::string data;

        uint64_t qword() const;
        const std::string str() const;
        std::string expanded() const;
        std::vector<std::string_view> multi() const;
    };
    struct Key {
        std::string name;
        std::vector<Key> keys;
        std::vector<Value> values;
        TimePoint lastWrite;
    };

public:
    bool load (Registry key, const std::string & root);

private:
    Registry m_registry;
    Key m_root;
};

} // namespace
