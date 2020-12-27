// Copyright Glen Knowles 2016 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// jsonstream.cpp - dim json
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   JsonStream
*
***/

//===========================================================================
JsonStream::JsonStream(IJsonStreamNotify * notify)
    : m_notify(*notify)
{}

//===========================================================================
JsonStream::~JsonStream()
{}

//===========================================================================
void JsonStream::clear() {
    m_line = 0;
    m_errmsg = nullptr;
}

//===========================================================================
bool JsonStream::parseMore(char src[]) {
    m_line = 0;
    m_errmsg = nullptr;
    m_base = m_heap.emplace<Detail::JsonParser>(this);
    m_notify.startDoc();
    if (!m_base->parse(src))
        return false;
    m_notify.endDoc();
    return true;
}

//===========================================================================
bool JsonStream::fail(const char errmsg[]) {
    m_errmsg = m_heap.strDup(errmsg);
    return false;
}

//===========================================================================
size_t JsonStream::errpos() const {
    return m_base->errpos();
}
