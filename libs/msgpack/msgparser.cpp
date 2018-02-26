// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// msgparser.cpp - dim msgpack
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using namespace MsgPack;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
constexpr size_t getSize(const void * ptr, size_t width) {
    switch (width) {
    case 1: return *(uint8_t*) ptr;
    case 2: return ntoh16(ptr);
    case 4: return ntoh32(ptr);
    case 8: return ntoh64(ptr);
    }
    assert(!"size has invalid bit-width");
    return 0;
}


/****************************************************************************
*
*   IMsgParser
*
***/

//===========================================================================
StreamParser::StreamParser(IParserNotify * notify)
    : m_notify(*notify)
{}

//===========================================================================
void StreamParser::clear() {
    m_bytes = 0;
    m_objects = 1;
}

//===========================================================================
error_code StreamParser::invalid(size_t pos) {
    m_pos += pos;
    return make_error_code(errc::invalid_argument);
}

//===========================================================================
error_code StreamParser::inProgress(size_t pos) {
    m_pos += pos;
    return make_error_code(errc::operation_in_progress);
}

//===========================================================================
error_code StreamParser::notifyArray(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    if (count - *pos < 1 + width)
        return inProgress(*pos);
    auto len = getSize(src.data() + *pos + 1, width);
    if (!m_notify.startArray(len))
        return invalid(*pos);
    *pos += (unsigned) width;
    m_objects += (unsigned) len - 1;
    return {};
}

//===========================================================================
error_code StreamParser::notifyMap(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    if (count - *pos < 1 + width)
        return inProgress(*pos);
    auto len = getSize(src.data() + *pos + 1, width);
    if (!m_notify.startMap(len))
        return invalid(*pos);
    *pos += (unsigned) width;
    m_objects += 2 * (unsigned) len - 1;
    return {};
}

//===========================================================================
error_code StreamParser::notifyPositive(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    if (count - *pos < 1 + width)
        return inProgress(*pos);
    if (!m_notify.positiveValue(
        (uint64_t) getSize(src.data() + *pos + 1, width))
    ) {
        return invalid(*pos);
    }
    *pos += (unsigned) width;
    m_objects -= 1;
    return {};
}

//===========================================================================
error_code StreamParser::notifyNegative(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    if (count - *pos < 1 + width)
        return inProgress(*pos);
    if (!m_notify.negativeValue(
        (int64_t) getSize(src.data() + *pos + 1, width))
    ) {
        return invalid(*pos);
    }
    *pos += (unsigned) width;
    m_objects -= 1;
    return {};
}

//===========================================================================
error_code StreamParser::notifyFloat(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto base = src.data();
    auto count = src.size();
    if (count - *pos < 1 + width)
        return inProgress(*pos);
    auto payload = (width == 4)
        ? (double) ntohf32(base + *pos + 1)
        : ntohf64(base + *pos + 1);
    if (!m_notify.value((double) payload))
        return invalid(*pos);
    *pos += (unsigned) width;
    m_objects -= 1;
    return {};
}

//===========================================================================
error_code StreamParser::notifyStr(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    auto prefix = width + 1;
    auto avail = count - *pos - prefix;
    if ((int) avail < 0)
        return inProgress(*pos);
    auto payload = getSize(src.data() + *pos + 1, width);
    if (payload <= avail) {
        if (!m_notify.value(src.substr(*pos + prefix, payload)))
            return invalid(*pos);
        *pos += (unsigned) prefix + (unsigned) payload - 1;
        m_objects -= 1;
        return {};
    } else if (payload <= 31) {
        return inProgress(*pos);
    }

    if (!m_notify.valuePrefix(src.substr(*pos + prefix, avail), true))
        return invalid(*pos);
    m_fmt = kStr8;
    m_bytes = unsigned(payload - avail);
    m_objects -= 1;
    return inProgress((int) count - 1);
}

//===========================================================================
error_code StreamParser::notifyExt(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    auto prefix = width + 2;
    auto avail = count - *pos - prefix;
    if ((int) avail < 0)
        return inProgress(*pos);
    auto payload = getSize(src.data() + *pos + 1, width);
    if (payload <= avail) {
        *pos += prefix + payload - 1;
        m_objects -= 1;
        return {};
    } else {
        m_fmt = kExt8;
        m_bytes = unsigned(payload - avail);
        m_objects -= 1;
        return inProgress((int) count - 1);
    }
}

//===========================================================================
error_code StreamParser::notifyFixExt(
    size_t * pos,
    string_view src,
    size_t width
) {
    auto count = src.size();
    if (count - *pos < 2 + width)
        return inProgress(*pos);
    *pos += 1 + (unsigned) width;
    m_objects -= 1;
    return {};
}

//===========================================================================
error_code StreamParser::parse(size_t * used, std::string_view src) {
    auto & pos = *used;
    auto count = src.size();
    error_code ec{};
    for (pos = 0; m_objects > 0 && pos < src.size(); ++pos) {
        if (m_bytes) {
            if (auto avail = count - pos; m_bytes > avail) {
                if (m_fmt == kStr8) {
                    if (!m_notify.valuePrefix(src.substr(pos), false))
                        return invalid(pos);
                }
                m_bytes -= (unsigned) avail;
                return inProgress((unsigned) count - 1);
            }

            if (m_fmt == kStr8) {
                if (!m_notify.value(src.substr(pos, m_bytes)))
                    return invalid(pos);
            }
            pos += m_bytes - 1;
            m_bytes = 0;
            continue;
        }

        uint8_t ch = src[pos];
        switch (ch >> 4) {
        case 0: case 1: case 2: case 3:
        case 4: case 5: case 6: case 7: // positive fixint
            if (!m_notify.positiveValue((uint64_t) ch & 0x7f))
                return invalid(pos);
            m_objects -= 1;
            continue;
        case 8: // fixmap
            if (!m_notify.startMap(ch & 0x0f))
                return invalid(pos);
            m_objects += 2 * (ch & 0x0f) - 1;
            continue;
        case 9: // fixarray
            if (!m_notify.startArray(ch & 0x0f))
                return invalid(pos);
            m_objects += (ch & 0x0f) - 1;
            continue;
        case 10: case 11: // fixstr
            if (auto len = ch & 0x1f; len <= count - pos - 1) {
                if (!m_notify.value(src.substr(pos + 1, len)))
                    return invalid(pos);
                pos += len;
                m_objects -= 1;
                continue;
            }
            return inProgress(pos);
        case 12: case 13:
            break;
        case 14: case 15: // negative fixint
            if (!m_notify.negativeValue((int64_t) ch & 0x1f))
                return invalid(pos);
            m_objects -= 1;
            continue;
        }

        switch (ch) {
        case kInvalid:
            return invalid(pos);
        case kArray16: ec = notifyArray(&pos, src, 2); break;
        case kArray32: ec = notifyArray(&pos, src, 4); break;
        case kMap16: ec = notifyMap(&pos, src, 2); break;
        case kMap32: ec = notifyMap(&pos, src, 4); break;
        case kNil:
            if (!m_notify.value(nullptr))
                return invalid(pos);
            m_objects -= 1;
            continue;
        case kFalse:
            if (!m_notify.value(false))
                return invalid(pos);
            m_objects -= 1;
            continue;
        case kTrue:
            if (!m_notify.value(true))
                return invalid(pos);
            m_objects -= 1;
            continue;
        case kInt8: ec = notifyNegative(&pos, src, 1); break;
        case kInt16: ec = notifyNegative(&pos, src, 2); break;
        case kInt32: ec = notifyNegative(&pos, src, 4); break;
        case kInt64: ec = notifyNegative(&pos, src, 8); break;
        case kUint8: ec = notifyPositive(&pos, src, 1); break;
        case kUint16: ec = notifyPositive(&pos, src, 2); break;
        case kUint32: ec = notifyPositive(&pos, src, 4); break;
        case kUint64: ec = notifyPositive(&pos, src, 8); break;
        case kFloat32: ec = notifyFloat(&pos, src, 4); break;
        case kFloat64: ec = notifyFloat(&pos, src, 8); break;
        case kStr8: case kBin8: ec = notifyStr(&pos, src, 1); break;
        case kStr16: case kBin16: ec = notifyStr(&pos, src, 2); break;
        case kStr32: case kBin32: ec = notifyStr(&pos, src, 4); break;
        case kExt8: ec = notifyExt(&pos, src, 1); break;
        case kExt16: ec = notifyExt(&pos, src, 2); break;
        case kExt32: ec = notifyExt(&pos, src, 4); break;
        case kFixExt1: ec = notifyFixExt(&pos, src, 1); break;
        case kFixExt2: ec = notifyFixExt(&pos, src, 2); break;
        case kFixExt4: ec = notifyFixExt(&pos, src, 4); break;
        case kFixExt8: ec = notifyFixExt(&pos, src, 8); break;
        case kFixExt16: ec = notifyFixExt(&pos, src, 16); break;
        }

        if (ec)
            return ec;
    }

    assert(m_bytes == 0);

    if (m_objects > 0)
        return inProgress(pos);

    assert(m_objects == 0);
    return {};
}
