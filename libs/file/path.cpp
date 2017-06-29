// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// path.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   Variables
*
***/



/****************************************************************************
*
*   Path
*
***/

//===========================================================================
Path::Path(std::string_view from) 
    : m_data(from)
{}

//===========================================================================
Path::Path(const fs::path & from) 
    : m_data(from.generic_u8string())
{}

//===========================================================================
fs::path Path::fsPath() const {
    return fs::u8path(m_data);
}

//===========================================================================
std::string_view Path::view() const {
    return m_data;
}
