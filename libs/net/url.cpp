// Copyright Glen Knowles 2018 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// url.cpp - dim net

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   URL Decode
*
***/

//===========================================================================
static string_view urlDecode(string_view src, ITempHeap & heap, bool query) {
    auto outlen = src.size();
    auto obase = heap.alloc<char>(outlen);
    auto optr = obase;
    auto base = src.data();
    auto eptr = base + outlen;
    for (auto ptr = base; ptr != eptr;) {
        int ch = *ptr++;
        if (ch == '%' && ptr + 2 <= eptr) {
            if (auto val = hexToByte(ptr[0], ptr[1]); val < 256) {
                *optr++ = (char) val;
                ptr += 2;
                continue;
            }
        } else if (ch == '+' && query) {
            ch = ' ';
        }
        *optr++ = (char) ch;
    }
    return {obase, size_t(optr - obase)};
}

//===========================================================================
string_view Dim::urlDecodePathComponent(string_view src, ITempHeap & heap) {
    return urlDecode(src, heap, false);
}

//===========================================================================
string_view Dim::urlDecodeQueryComponent(
    string_view src,
    ITempHeap & heap
) {
    return urlDecode(src, heap, true);
}


/****************************************************************************
*
*   URL Encode
*
***/

//===========================================================================
static bool escapeInPath(char ch) {
    switch (ch) {
    default:
        return true;

    // unreserved
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
    case '-': case '.': case '_': case '~':
    // sub-delims
    case '!': case '$': case '&': case '\'': case '(': case ')':
    case '*': case '+': case ',': case ';': case '=':
    // additional pchar
    case ':': case '@':
        return false;
    }
}

//===========================================================================
static bool escapeInQuery(char ch) {
    switch (ch) {
    default:
        return true;

    // unreserved
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
    case '-': case '.': case '_': case '~':
    // sub-delims
    case '!': case '$': case '&': case '\'': case '(': case ')':
    case '*': case '+': case ',': case ';': case '=':
    // additional pchar
    case ':': case '@':
    // additional query
    case '/': case '?':
        return false;
    }
}

//===========================================================================
static size_t urlEncode(
    void * vout,
    size_t outLen,
    string_view src,
    bool query
) {
    auto out = (char *) vout;
    for (auto && ch : src) {
        if (query ? escapeInQuery(ch) : escapeInPath(ch)) {
            if (outLen < 3)
                break;
            *out++ = '%';
            *out++ = hexFromNibble(uint8_t(ch) >> 4);
            *out++ = hexFromNibble(uint8_t(ch) & 0xf);
            outLen -= 3;
        } else {
            *out++ = ch;
            outLen -= 1;
        }
        if (!outLen)
            break;
    }
    return out - (char *) vout;
}

//===========================================================================
size_t Dim::urlEncodePathComponentLen(std::string_view src) {
    size_t num = 0;
    for (auto && ch : src)
        num += escapeInPath(ch) ? 3 : 1;
    return num;
}

//===========================================================================
size_t Dim::urlEncodePathComponent(span<char> out, string_view src) {
    return urlEncode(out.data(), out.size(), src, false);
}

//===========================================================================
string Dim::urlEncodePathComponent(string_view src) {
    string out;
    out.resize(urlEncodePathComponentLen(src));
    urlEncode(out.data(), out.size(), src, false);
    return out;
}

//===========================================================================
size_t Dim::urlEncodeQueryComponentLen(std::string_view src) {
    size_t num = 0;
    for (auto && ch : src)
        num += escapeInQuery(ch) ? 3 : 1;
    return num;
}

//===========================================================================
size_t Dim::urlEncodeQueryComponent(span<char> out, string_view src) {
    return urlEncode(out.data(), out.size(), src, true);
}

//===========================================================================
string Dim::urlEncodeQueryComponent(string_view src) {
    string out;
    out.resize(urlEncodeQueryComponentLen(src));
    urlEncode(out.data(), out.size(), src, true);
    return out;
}


/****************************************************************************
*
*   Query strings
*
***/

//===========================================================================
static void addSeg(
    HttpQuery * hp,
    ITempHeap & heap,
    const char *& base,
    const char * eptr,
    unsigned & pcts
) {
    auto count = size_t(eptr - base - 1);
    auto seg = heap.emplace<HttpPathValue>();
    hp->pathSegs.link(seg);
    seg->value = {base, count};
    if (pcts) {
        seg->value = urlDecodePathComponent(seg->value, heap);
        pcts = 0;
    }
    base = eptr;
}

//===========================================================================
static void setPath(HttpQuery * hp, ITempHeap & heap) {
    auto found = size_t{0};
    for (auto && seg : hp->pathSegs)
        found += seg.value.size() + 1;
    auto * out = heap.alloc<char>(found);
    hp->path = {out, found};
    for (auto && seg : hp->pathSegs) {
        *out++ = '/';
        memcpy(out, seg.value.data(), seg.value.size());
        out += seg.value.size();
    }
}

//===========================================================================
static HttpPathParam * addParam(
    HttpQuery * hp,
    ITempHeap & heap,
    const char *& base,
    const char * eptr,
    unsigned & pcts
) {
    auto count = size_t(eptr - base - 1);
    auto param = heap.emplace<HttpPathParam>();
    hp->parameters.link(param);
    param->name = {base, count};
    if (pcts) {
        param->name = urlDecodeQueryComponent(param->name, heap);
        pcts = 0;
    }
    base = eptr;
    return param;
}

//===========================================================================
static void addParamValue(
    HttpPathParam * param,
    ITempHeap & heap,
    const char *& base,
    const char * eptr,
    unsigned & pcts
) {
    auto count = size_t(eptr - base - 1);
    auto val = heap.emplace<HttpPathValue>();
    param->values.link(val);
    val->value = {base, count};
    if (pcts) {
        val->value = urlDecodeQueryComponent(val->value, heap);
        pcts = 0;
    }
    base = eptr;
}

//===========================================================================
HttpQuery * Dim::urlParseHttpPath(string_view path, ITempHeap & heap) {
    auto qry = heap.emplace<HttpQuery>();
    if (path.empty())
        return qry;
    if (path.size() == 1 && path[0] == '*') {
        qry->asterisk = true;
        return qry;
    }
    auto ptr = path.data();
    auto eptr = ptr + path.size();

    unsigned pcts = 0;
    unsigned char ch = *ptr++;
    auto base = ptr;
    if (ch == '/')
        goto SEGMENTS;
    if (ch == '?')
        urlAddQueryString(qry, path, heap);
    return qry;

SEGMENTS:
    while (ptr != eptr) {
        ch = *ptr++;
        if (ch == '/') {
            addSeg(qry, heap, base, ptr, pcts);
        } else if (ch == '?') {
            addSeg(qry, heap, base, ptr, pcts);
            setPath(qry, heap);
            urlAddQueryString(qry, string_view(ptr, eptr - ptr), heap);
            return qry;
        } else if (ch == '%') {
            pcts += 1;
        } else if (ch == '#' || ch == 0) {
            ptr -= 1;
            break;
        }
    }
    addSeg(qry, heap, base, ptr + 1, pcts);
    setPath(qry, heap);
    return qry;
}

//===========================================================================
void Dim::urlAddQueryString(HttpQuery * out, string_view src, ITempHeap & heap) {
    if (src.empty())
        return;

    auto ptr = src.data();
    auto eptr = ptr + src.size();

    HttpPathParam * param = nullptr;
    unsigned pcts = 0;
    unsigned char ch = *ptr;
    if (ch == '?')
        ptr += 1;
    auto base = ptr;

QUERY_PARAM:
    while (ptr != eptr) {
        ch = *ptr++;
        if (ch == '&') {
            param = addParam(out, heap, base, ptr, pcts);
        } else if (ch == '=') {
            param = addParam(out, heap, base, ptr, pcts);
            goto QUERY_VALUE;
        } else if (ch == '%') {
            pcts += 1;
        } else if (ch == '#' || ch == 0) {
            ptr -= 1;
            break;
        }
    }
    addParam(out, heap, base, ptr + 1, pcts);
    return;

QUERY_VALUE:
    while (ptr != eptr) {
        ch = *ptr++;
        if (ch == '&') {
            addParamValue(param, heap, base, ptr, pcts);
            goto QUERY_PARAM;
        } else if (ch == '%') {
            pcts += 1;
        } else if (ch == '#' || ch == 0) {
            ptr -= 1;
            break;
        }
    }
    addParamValue(param, heap, base, ptr + 1, pcts);
    return;
}


