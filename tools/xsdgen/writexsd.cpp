// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// writexsd.cpp - xsdgen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/


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


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
bool writeXsd(IXBuilder * out, const Schema & schema) {
    out->start("schema")
        .attr("xmlns", "http://www.w3.org.2001.XMLSchema")
        .attr("xmlns:def", "http://github.com/gknowles/dimapp/xsdgen.xsd")
        .attr("targetNamespace", "http://github.com/gknowles/dimapp/xsdgen.xsd")
        .attr("elementFormDefault", "qualified");
    out->start("element")
        .attr("name", "Schema")
        .attr("type", "def:tSchema")
        .end();
    // complexType per element
    out->end();
    return false;
}
