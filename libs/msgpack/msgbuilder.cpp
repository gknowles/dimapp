// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// msgbuilder.cpp - dim msgpack
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using namespace MsgPack;


/****************************************************************************
*
*   IBuilder
*
***/

enum class IBuilder::State : int {
    kValue,
    kArray,
    kElement,
    kElementValue,
};

//===========================================================================
IBuilder::IBuilder()
    : m_state(State::kValue)
{}

//===========================================================================
void IBuilder::clear() {
    m_state = State::kValue;
    m_stack.clear();
}

//===========================================================================
bool IBuilder::pop() {
    assert(m_remaining);
    if (--m_remaining)
        return false;
    m_state = m_stack.back().first;
    m_remaining = m_stack.back().second;
    m_stack.pop_back();
    return true;
}

//===========================================================================
IBuilder & IBuilder::array(size_t count) {
    assert(unsigned(count) == count);
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        pop();
        break;
    case State::kElement:
        assert(!"adding array when element key was expected");
        return *this;
    case State::kElementValue:
        assert(m_remaining);
        if (!pop())
            m_state = State::kElement;
        break;
    }

    m_stack.emplace_back(m_state, m_remaining);
    m_remaining = unsigned(count);
    m_state = State::kArray;
    if (count <= 0xf) {
        append(char(count | kFixArrayMask));
        if (!count) {
            m_remaining = 1;
            pop();
        }
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
IBuilder & IBuilder::map(size_t count) {
    assert(unsigned(count) == count);
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        pop();
        break;
    case State::kElement:
        assert(!"adding map when element key was expected");
        return *this;
    case State::kElementValue:
        if (!pop())
            m_state = State::kElement;
        break;
    }

    m_stack.emplace_back(m_state, m_remaining);
    m_remaining = unsigned(count);
    m_state = State::kElement;
    if (count <= 0xf) {
        append(char(count | kFixMapMask));
        if (!count) {
            m_remaining = 1;
            pop();
        }
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
IBuilder & IBuilder::element(string_view key) {
    assert(m_state == State::kElement);
    assert(m_remaining);
    m_state = State::kElementValue;

    appendString(key);
    return *this;
}

//===========================================================================
IBuilder & IBuilder::value(string_view val) {
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        pop();
        break;
    case State::kElement:
        assert(!"adding value when element key was expected");
        return *this;
    case State::kElementValue:
        if (!pop())
            m_state = State::kElement;
        break;
    }

    appendString(val);
    return *this;
}

//===========================================================================
IBuilder & IBuilder::valueRaw(string_view val) {
    switch (m_state) {
    case State::kValue:
        break;
    case State::kArray:
        pop();
        break;
    case State::kElement:
        assert(!"adding value when element key was expected");
        return *this;
    case State::kElementValue:
        if (!pop())
            m_state = State::kElement;
        break;
    }

    append(val);
    return *this;
}

//===========================================================================
IBuilder & IBuilder::value(const char val[]) {
    return val ? value(string_view{val}) : value(nullptr);
}

//===========================================================================
IBuilder & IBuilder::value(bool val) {
    uint8_t out[1] = { val ? kTrue : kFalse };
    return valueRaw({(char *) &out, ::size(out)});
}

//===========================================================================
IBuilder & IBuilder::value(double val) {
    if (auto ival = int(val); ival == val) {
        return value(ival);
    } else if (auto fval = float(val); fval == val) {
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
IBuilder & IBuilder::value(int64_t val) {
    if (val >= 0)
        return value(uint64_t(val));

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
IBuilder & IBuilder::value(uint64_t val) {
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
IBuilder & IBuilder::value(nullptr_t) {
    uint8_t out[1] = { kNil };
    return valueRaw({(char *) &out, ::size(out)});
}

//===========================================================================
void IBuilder::appendString(string_view val) {
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
*   Builder
*
***/

//===========================================================================
void Builder::clear() {
    IBuilder::clear();
    m_buf.clear();
}

//===========================================================================
void Builder::append(string_view val) {
    m_buf.append(val);
}

//===========================================================================
void Builder::append(char val) {
    m_buf.append(1, val);
}

//===========================================================================
size_t Builder::size() const {
    return m_buf.size();
}
