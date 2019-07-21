// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// cron.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

vector<TokenTable::Token> const Cron::s_macros = {
    { Cron::kYearly, "yearly" },
    { Cron::kYearly, "annually" },
    { Cron::kMonthly, "monthly" },
    { Cron::kWeekly, "weekly" },
    { Cron::kDaily, "daily" },
    { Cron::kDaily, "midnight" },
    { Cron::kHourly, "hourly" },
};
TokenTable const Cron::s_macroTbl(s_macros);

TokenTable::Token const s_wdays[] = {
    { 0, "sun" },
    { 1, "mon" },
    { 2, "tue" },
    { 3, "wed" },
    { 4, "thu" },
    { 5, "fri" },
    { 6, "sat" },
};
TokenTable const Cron::s_wdayTbl(s_wdays);

TokenTable::Token const s_months[] = {
    {  1, "jan" },
    {  2, "feb" },
    {  3, "mar" },
    {  4, "apr" },
    {  5, "may" },
    {  6, "jun" },
    {  7, "jul" },
    {  8, "aug" },
    {  9, "sep" },
    { 10, "oct" },
    { 11, "nov" },
    { 12, "dec" },
};
TokenTable const Cron::s_monTbl(s_months);


/****************************************************************************
*
*   Helpers
*
***/
