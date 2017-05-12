// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winplatform.cpp - dim windows platform
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
*   Helpers
*
***/


/****************************************************************************
*
*   User info
*
***/

namespace {
class HtmlAccount : public IHttpRouteNotify {
	void onHttpRequest(
		unsigned reqId,
		unordered_multimap<string_view, string_view> & params,
		HttpRequest & msg
	) override;
};
} // namespace

//===========================================================================
static void addSidRow(JBuilder & out, SID_AND_ATTRIBUTES & sa) {
	DWORD nameLen;
	DWORD domLen;
	SID_NAME_USE use;
	if (!LookupAccountSid(
		NULL,
		sa.Sid,
		NULL, &nameLen,
		NULL, &domLen,
		&use
	)) {
		logMsgCrash() << "LookupAccountSid(NULL): " << WinError{};
	}
    nameLen += 1;
    domLen += 1;
	string name(nameLen, 0);
	string dom(domLen, 0);
	if (!LookupAccountSid(
		NULL,
		sa.Sid,
		name.data(), &nameLen,
		dom.data(), &domLen,
		&use
	)) {
		logMsgCrash() << "LookupAccountSid: " << WinError{};
	}
    name.resize(nameLen);
    dom.resize(domLen);
	out.object();
    out.member("attrs", (uint64_t) sa.Attributes);
    out.member("name", name);
    out.member("domain", dom);
    out.member("type", use);
    out.end();
}

//===========================================================================
void HtmlAccount::onHttpRequest(
	unsigned reqId,
	unordered_multimap<string_view, string_view> & params,
	HttpRequest & msg
) {
	auto proc = GetCurrentProcess();
	HANDLE token;
	if (!OpenProcessToken(proc, TOKEN_QUERY, &token))
		logMsgCrash() << "OpenProcessToken: " << WinError{};
	DWORD len;
	BOOL result = GetTokenInformation(token, TokenUser, NULL, 0, &len);
    WinError err;
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
		logMsgCrash() << "GetTokenInformation(TokenUser, NULL): " 
            << WinError{};
    }
    auto usr = unique_ptr<TOKEN_USER>((TOKEN_USER *) malloc(len));
	if (!GetTokenInformation(token, TokenUser, usr.get(), len, &len))
		logMsgCrash() << "GetTokenInformation(TokenUser): " << WinError{};
	result = GetTokenInformation(token, TokenGroups, NULL, 0, &len);
    err.set();
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
		logMsgCrash() << "GetTokenInformation(TokenGroups, NULL): " 
            << WinError{};
    }
	auto grps = unique_ptr<TOKEN_GROUPS>((TOKEN_GROUPS *) malloc(len));
	if (!GetTokenInformation(token, TokenGroups, grps.get(), len, &len))
		logMsgCrash() << "GetTokenInformation(TokenGroups): " << WinError{};

	HttpResponse res;
	JBuilder bld(res.body());
	bld.array();
	addSidRow(bld, usr->User);
	auto * ptr = grps->Groups,
		*eptr = ptr + grps->GroupCount;
	for (; ptr != eptr; ++ptr) {
		addSidRow(bld, *ptr);
	}
	bld.end();
	res.addHeader(kHttpContentType, "text/html");
	res.addHeader(kHttp_Status, "200");
	httpRouteReply(reqId, res);
}


static HtmlAccount s_account;


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iPlatformInitialize() {
    winErrorInitialize();
    winIocpInitialize();
    winAppInitialize();
}

//===========================================================================
void Dim::iPlatformConfigInitialize() {
    winAppConfigInitialize();

    if (appFlags() & fAppWithWebAdmin)
        httpRouteAdd(&s_account, "/srv/account.json");
}
