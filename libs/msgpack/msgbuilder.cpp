// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// jbuilder.cpp - dim msgpack
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

enum Format : uint8_t {
    kFixArrayMask = 0x90,
    kArray16 = 0xdc,
    kArray32 = 0xdd,
    kFixMapMask = 0x80,
    kMap16 = 0xde,
    kMap32 = 0xdf,
    kNil = 0xc0,
    kFalse = 0xc2,
    kTrue = 0xc3,
    kPositiveFixIntMask = 0x00,
    kNegativeFixIntMask = 0xe0,
    kInt8 = 0xd0,
    kInt16 = 0xd1,
    kInt32 = 0xd2,
    kInt64 = 0xd3,
    kUint8 = 0xcc,
    kUint16 = 0xcd,
    kUint32 = 0xce,
    kUint64 = 0xcf,
    kFloat32 = 0xca,
    kFloat64 = 0xcb,
    kFixStrMask = 0xa0,
    kStr8 = 0xd9,
    kStr16 = 0xda,
    kStr32 = 0xdb,
    kBin8 = 0xc4,
    kBin16 = 0xc5,
    kBin32 = 0xc6,
    kExt8 = 0xc7,
    kExt16 = 0xc8,
    kExt32 = 0xc9,
    kFixExt1 = 0xd4,
    kFixExt2 = 0xd5,
    kFixExt4 = 0xd6,
    kFixExt8 = 0xd7,
    kkFixExt16 = 0xd8,
};

} // namespace


/****************************************************************************
*
*   IMsgBuilder
*
***/

enum class IMsgBuilder::State : int {
    kValue,
    kArray,
    kElement,
    kElementValue,
};

//===========================================================================
IMsgBuilder::IMsgBuilder()
    : m_state(State::kValue) {}

//===========================================================================
void IMsgBuilder::clear() {
    m_state = State::kValue;
    m_stack.clear();
}

//===========================================================================
IMsgBuilder & IMsgBuilder::array(size_t count) {
    assert(unsigned(count) == count);
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        }
        break;
    case State::kElement:
        assert(!"adding array when element key was expected");
        return *this;
    case State::kElementValue:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        } else {
            m_state = State::kElement;
        }
        break;
    }

    m_stack.push_back({m_state, m_remaining});
    m_remaining = unsigned(count);
    m_state = State::kArray;
    if (count <= 0xf) {
        append(char(count | kFixArrayMask));
    } else if (count <= 0xffff) {
        uint8_t out[3] = { kArray16, uint8_t(count >> 8), uint8_t(count) };
        append(string_view{(char *) out, ::size(out)});
    } else {
        assert(count <= 0xffff'ffff);
        uint8_t out[5] = {
            kArray32,
            uint8_t(count >> 24),
            uint8_t(count >> 16),
            uint8_t(count >> 8),
            uint8_t(count)
        };
        append(string_view{(char *) out, ::size(out)});
    }
    return *this;
}

//===========================================================================
IMsgBuilder & IMsgBuilder::map(size_t count) {
    assert(unsigned(count) == count);
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        }
        break;
    case State::kElement:
        assert(!"adding map when element key was expected");
        return *this;
    case State::kElementValue:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        } else {
            m_state = State::kElement;
        }
        break;
    }

    m_stack.push_back({m_state, m_remaining});
    m_remaining = unsigned(count);
    m_state = State::kElement;
    if (count <= 0xf) {
        append(char(count | kFixMapMask));
    } else if (count <= 0xffff) {
        uint8_t out[3] = { kMap16, uint8_t(count >> 8), uint8_t(count) };
        append(string_view{(char *) out, ::size(out)});
    } else {
        assert(count <= 0xffff'ffff);
        uint8_t out[5] = {
            kMap32,
            uint8_t(count >> 24),
            uint8_t(count >> 16),
            uint8_t(count >> 8),
            uint8_t(count)
        };
        append(string_view{(char *) out, ::size(out)});
    }
    return *this;
}

//===========================================================================
IMsgBuilder & IMsgBuilder::element(string_view key) {
    assert(m_state == State::kElement);
    assert(m_remaining);
    m_state = State::kElementValue;

    appendString(key);
    return *this;
}

//===========================================================================
IMsgBuilder & IMsgBuilder::value(string_view val) {
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        }
        break;
    case State::kElement:
        assert(!"adding value when element key was expected");
        return *this;
    case State::kElementValue:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        } else {
            m_state = State::kElement;
        }
        break;
    }

    appendString(val);
    return *this;
}

//===========================================================================
IMsgBuilder & IMsgBuilder::valueRaw(string_view val) {
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        }
        break;
    case State::kElement:
        assert(!"adding value when element key was expected");
        return *this;
    case State::kElementValue:
        assert(m_remaining);
        if (!--m_remaining) {
            m_state = m_stack.back().first;
            m_remaining = m_stack.back().second;
        } else {
            m_state = State::kElement;
        }
        break;
    }

    append(val);
    return *this;
}

//===========================================================================
IMsgBuilder & IMsgBuilder::value(const char val[]) {
    return value(string_view{val});
}

//===========================================================================
IMsgBuilder & IMsgBuilder::value(bool val) {
    uint8_t out[1] = { val ? kTrue : kFalse };
    return valueRaw({(char *) &out, ::size(out)});
}

//===========================================================================
IMsgBuilder & IMsgBuilder::value(double val) {
    if (auto fval = float(val); fval == val) {
        unsigned char * in = (unsigned char *) &fval;
        uint8_t out[5] = { kFloat32, in[3], in[2], in[1], in[0] };
        return valueRaw({(char *) &out, ::size(out)});
    } else {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[9] = {
            kFloat64, in[7], in[6], in[5], in[4], in[3], in[2], in[1], in[0]
        };
        return valueRaw({(char *) &out, ::size(out)});
    }
}

//===========================================================================
IMsgBuilder & IMsgBuilder::ivalue(int64_t val) {
    if (val >= 0)
        return uvalue(val);

    if (val >= -0x1f) {
        uint8_t out[1] = { uint8_t(kNegativeFixIntMask | -val) };
        return valueRaw({(char *) &out, ::size(out)});
    } else if (val >= -0x7f) {
        uint8_t out[2] = { kInt8, uint8_t(val) };
        return valueRaw({(char *) &out, ::size(out)});
    } else if (val >= -0x7fff) {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[3] = { kInt16, in[1], in[0] };
        return valueRaw({(char *) &out, ::size(out)});
    } else if (val >= -0x7fff'ffff) {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[5] = { kInt32, in[3], in[2], in[1], in[0] };
        return valueRaw({(char *) &out, ::size(out)});
    } else {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[9] = {
            kInt64, in[7], in[6], in[5], in[4], in[3], in[2], in[1], in[0]
        };
        return valueRaw({(char *) &out, ::size(out)});
    }
}

//===========================================================================
IMsgBuilder & IMsgBuilder::uvalue(uint64_t val) {
    if (val <= 0x7f) {
        uint8_t out[1] = { uint8_t(val) };
        return valueRaw({(char *) &out, ::size(out)});
    } else if (val <= 0xff) {
        uint8_t out[2] = { kUint8, uint8_t(val) };
        return valueRaw({(char *) &out, ::size(out)});
    } else if (val <= 0xffff) {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[3] = { kUint16, in[1], in[0] };
        return valueRaw({(char *) &out, ::size(out)});
    } else if (val <= 0xffff'ffff) {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[5] = { kUint32, in[3], in[2], in[1], in[0] };
        return valueRaw({(char *) &out, ::size(out)});
    } else {
        unsigned char * in = (unsigned char *) &val;
        uint8_t out[9] = {
            kUint64, in[7], in[6], in[5], in[4], in[3], in[2], in[1], in[0]
        };
        return valueRaw({(char *) &out, ::size(out)});
    }
}

//===========================================================================
IMsgBuilder & IMsgBuilder::value(nullptr_t) {
    uint8_t out[1] = { kNil };
    return valueRaw({(char *) &out, ::size(out)});
}

//===========================================================================
void IMsgBuilder::appendString(string_view val) {
    auto count = val.size();
    if (count <= 0x1f) {
        append(kFixStrMask | uint8_t(count));
    } else if (count <= 0xff) {
        uint8_t out[2] = { kStr8, uint8_t(count) };
        append({(char *) &out, ::size(out)});
    } else if (count <= 0xffff) {
        unsigned char * in = (unsigned char *) &count;
        uint8_t out[3] = { kStr16, in[1], in[0] };
        append({(char *) &out, ::size(out)});
    } else {
        assert(count <= 0xffff'ffff);
        unsigned char * in = (unsigned char *) &count;
        uint8_t out[5] = { kStr32, in[3], in[2], in[1], in[0] };
        append({(char *) &out, ::size(out)});
    }
    append(val);
}


/****************************************************************************
*
*   MsgBuilder
*
***/

//===========================================================================
void MsgBuilder::clear() {
    IMsgBuilder::clear();
    m_buf.clear();
}

//===========================================================================
void MsgBuilder::append(string_view val) {
    m_buf.append(val);
}

//===========================================================================
void MsgBuilder::append(char val) {
    m_buf.append(1, val);
}

//===========================================================================
size_t MsgBuilder::size() const {
    return m_buf.size();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
IMsgBuilder & Dim::operator<<(IMsgBuilder & out, std::string_view val) {
    return out.value(val);
}

//===========================================================================
IMsgBuilder & Dim::operator<<(IMsgBuilder & out, int64_t val) {
    return out.ivalue(val);
}

//===========================================================================
IMsgBuilder & Dim::operator<<(IMsgBuilder & out, uint64_t val) {
    return out.uvalue(val);
}

//===========================================================================
IMsgBuilder & Dim::operator<<(IMsgBuilder & out, bool val) {
    return out.value(val);
}

//===========================================================================
IMsgBuilder & Dim::operator<<(IMsgBuilder & out, double val) {
    return out.value(val);
}

//===========================================================================
IMsgBuilder & Dim::operator<<(IMsgBuilder & out, nullptr_t) {
    return out.value(nullptr);
}
