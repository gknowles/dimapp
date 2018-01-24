// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// url.cpp - dim net

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

} // namespace


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static string_view urlDecode(string_view src, ITempHeap & heap, bool query) {
    auto outlen = src.size();
    auto obase = heap.alloc(outlen);
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
        seg->value = urlDecode(seg->value, heap, false);
        pcts = 0;
    }
    base = eptr;
}

//===========================================================================
static void setPath(HttpQuery * hp, ITempHeap & heap) {
    auto found = size_t{0};
    for (auto && seg : hp->pathSegs)
        found += seg.value.size() + 1;
    auto * out = heap.alloc(found);
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
        param->name = urlDecode(param->name, heap, true);
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
        val->value = urlDecode(val->value, heap, true);
        pcts = 0;
    }
    base = eptr;
}


/****************************************************************************
*
*   Public API
*
***/

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

    HttpPathParam * param{nullptr};
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

//===========================================================================
size_t Dim::urlEncodePathComponent(
    char * out,
    size_t outLen,
    string_view src
);

//===========================================================================
size_t Dim::urlEncodeQueryComponent(
    char * out,
    size_t outLen,
    string_view src
);

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
