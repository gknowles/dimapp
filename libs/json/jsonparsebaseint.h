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
    const char * m_base{nullptr};
    char * m_cur{nullptr};

    bool m_minus{false};
    int64_t m_int{0};
    int m_frac{0};
    bool m_expMinus{false};
    int m_exp{0};

    char32_t m_hexchar{0};
};

//===========================================================================
inline JsonParserBase::JsonParserBase(JsonStream * parser)
    : m_parser(*parser)
    , m_notify(parser->notify())
{}

} // namespace
