// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// jsonparsebaseint.h - dim json

#include "json/json.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   JsonParserBase
*
***/

struct JsonParserBase {
    JsonParserBase(JsonStream * parser);

    JsonStream & m_parser;
    IJsonStreamNotify & m_notify;

    std::string_view m_name;
    char const * m_base{};
    char * m_cur{};

    bool m_minus{};
    int64_t m_int{};
    int m_frac{};
    bool m_expMinus{};
    int m_exp{};

    char32_t m_hexchar{};
};

//===========================================================================
inline JsonParserBase::JsonParserBase(JsonStream * parser)
    : m_parser(*parser)
    , m_notify(parser->notify())
{}

} // namespace
