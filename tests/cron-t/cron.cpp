// Copyright Glen Knowles 2019 - 2022.
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
const TokenTable Cron::s_macroTbl(s_macros);

const TokenTable::Token s_wdays[] = {
    { 0, "sun" },
    { 1, "mon" },
    { 2, "tue" },
    { 3, "wed" },
    { 4, "thu" },
    { 5, "fri" },
    { 6, "sat" },
};
const TokenTable Cron::s_wdayTbl(s_wdays);

const TokenTable::Token s_months[] = {
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
const TokenTable Cron::s_monTbl(s_months);


/****************************************************************************
*
*   Helpers
*
***/
