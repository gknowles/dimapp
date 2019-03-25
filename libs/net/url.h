// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// url.h - dim http
#pragma once

#include "cppconf/cppconf.h"

#include "core/core.h"

namespace Dim {


/****************************************************************************
*
*   HttpQuery
*
*   Examples:
*       *
*       /
*       /a/b?x=1&y&z=%33
*       /c?&w
*
***/

struct HttpPathValue : ListBaseLink<> {
    std::string_view value;
};
struct HttpPathParam : ListBaseLink<> {
    std::string_view name;
    List<HttpPathValue> values;
};

struct HttpQuery {
    bool asterisk{false};
    std::string_view path;
    List<HttpPathValue> pathSegs;
    List<HttpPathParam> parameters;
};

HttpQuery * urlParseHttpPath(std::string_view path, ITempHeap & heap);
void urlAddQueryString(HttpQuery * out, std::string_view src, ITempHeap & heap);

size_t urlEncodePathComponentLen(std::string_view src);
size_t urlEncodeQueryComponentLen(std::string_view src);
size_t urlEncodePathComponent(char * out, size_t outLen, std::string_view src);
size_t urlEncodeQueryComponent(char * out, size_t outLen, std::string_view src);
std::string_view urlDecodePathComponent(std::string_view src, ITempHeap & heap);
std::string_view urlDecodeQueryComponent(
    std::string_view src,
    ITempHeap & heap
);

} // namespace
