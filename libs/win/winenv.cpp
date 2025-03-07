// Copyright Glen Knowles 2017 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// winenv.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private declarations
*
***/

//---------------------------------------------------------------------------
// Windows API declarations not available from Windows.h
//---------------------------------------------------------------------------
#pragma pack(push)
#pragma pack()
struct DIM_OSVERSIONINFOEXW {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WCHAR szCSDVersion[ 128 ];     // Maintenance string for PSS usage
    WORD  wServicePackMajor;
    WORD  wServicePackMinor;
    WORD  wSuiteMask;
    BYTE  wProductType;
    BYTE  wReserved;
};
#pragma pack(pop)
using RtlGetVersionFn = NTSTATUS(WINAPI *)(DIM_OSVERSIONINFOEXW * info);


/****************************************************************************
*
*   Variables
*
***/

static RtlGetVersionFn s_RtlGetVersion;
static EnvMemoryConfig s_memCfg;
static unsigned s_numProcessors;
static string s_procAcct;


/****************************************************************************
*
*   Private API
*
***/

//===========================================================================
void Dim::winEnvInitialize() {
    winLoadProc(&s_RtlGetVersion, "ntdll", "RtlGetVersion");

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    s_memCfg = {};
    s_memCfg.pageSize = info.dwPageSize;
    s_memCfg.allocAlign = info.dwAllocationGranularity;
    if (winEnablePrivilege(SE_LOCK_MEMORY_NAME)) {
        // We have the right to allocate large pages, now get how big they are
        // and see if they are supported at all (non-zero).
        s_memCfg.minLargeAlloc = GetLargePageMinimum();
    }

    s_numProcessors = info.dwNumberOfProcessors;

    wstring out;
    DWORD bufLen = 0;
    while (!GetUserNameW(out.data(), &bufLen)) {
        WinError err;
        if (err == ERROR_INSUFFICIENT_BUFFER) {
            out.resize(bufLen);
            continue;
        }
        logMsgFatal() << "GetUserNameW: " << err;
    }
    s_procAcct = toString(out);
}


/****************************************************************************
*
*   System Information
*
***/

//===========================================================================
const EnvMemoryConfig & Dim::envMemoryConfig() {
    assert(s_numProcessors);
    return s_memCfg;
}

//===========================================================================
unsigned Dim::envProcessors() {
    assert(s_numProcessors);
    return s_numProcessors;
}

//===========================================================================
DiskAlignment Dim::envDiskAlignment(string_view path) {
    HANDLE vh = INVALID_HANDLE_VALUE;
    DiskAlignment out = {};

    for (;;) {
        wchar_t mountPoint[MAX_PATH + 1];
        if (!GetVolumePathNameW(
            toWstring(path).c_str(),
            mountPoint,
            (DWORD) size(mountPoint)
        )) {
            logMsgFatal() << "GetVolumePathNameW(" << path << "): "
                << WinError{};
            break;
        }
        wchar_t vname[MAX_PATH + 1];
        if (!GetVolumeNameForVolumeMountPointW(
            mountPoint,
            vname,
            (DWORD) size(vname)
        )) {
            logMsgFatal() << "GetVolumeNameForVolumeMountPointW("
                << utf8(mountPoint) << "): " << WinError{};
            break;
        }

        // CreateFileW needs volume name without trailing slash to open volume,
        // with the slash it's the root directory of the volume.
        vname[wcslen(vname) - 1] = 0;
        vh = CreateFileW(
            vname,
            0,      // access - NONE
            0,      // share mode
            NULL,   // security attributes,
            OPEN_EXISTING,
            0,      // flags and attributes
            NULL    // template
        );
        if (vh == INVALID_HANDLE_VALUE) {
            logMsgFatal() << "CreateFileW(" << utf8(vname) << "): "
                << WinError();
            break;
        }

        STORAGE_PROPERTY_QUERY qry = {};
        qry.PropertyId = StorageAccessAlignmentProperty;
        qry.QueryType = PropertyStandardQuery;
        STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR sa = {};
        DWORD bytes;
        if (!DeviceIoControl(
            vh,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &qry,
            (DWORD) sizeof qry,
            &sa,
            (DWORD) sizeof sa,
            &bytes,
            NULL    // overlapped
        )) {
            logMsgFatal() << "DeviceIoControl(StorageAccessAlignment): "
                << WinError{};
            break;
        }

        out.cacheLine = sa.BytesPerCacheLine;
        out.cacheOffset = sa.BytesOffsetForCacheAlignment;
        out.logicalSector = sa.BytesPerLogicalSector;
        out.physicalSector = sa.BytesPerPhysicalSector;
        out.sectorOffset = sa.BytesOffsetForSectorAlignment;
        break;
    }

    if (vh != INVALID_HANDLE_VALUE)
        CloseHandle(vh);
    return out;
}

//===========================================================================
DiskSpace Dim::envDiskSpace(string_view path) {
    ULARGE_INTEGER avail;
    ULARGE_INTEGER total;
    if (!GetDiskFreeSpaceExW(
        toWstring(path).c_str(),
        &avail,
        &total,
        NULL        // total free, may be >avail to this process
    )) {
        logMsgFatal() << "GetDiskFreeSpaceExW(" << path << "): " << WinError{};
    }
    DiskSpace out;
    out.avail = avail.QuadPart;
    out.total = total.QuadPart;
    return out;
}

//===========================================================================
string Dim::envComputerName() {
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD bufLen = (DWORD) size(buf);
    if (!GetComputerNameW(buf, &bufLen))
        logMsgFatal() << "GetComputerNameW: " << WinError();
    return toString(buf);
}

//===========================================================================
string Dim::envComputerDnsName() {
    wstring wbuf;
    DWORD bufLen = (DWORD) size(wbuf);
    if (!GetComputerNameExW(
        ComputerNameDnsFullyQualified,
        NULL,
        &bufLen
    )) {
        WinError err;
        if (err != ERROR_MORE_DATA)
            logMsgFatal() << "GetComputerNameExW(NULL): " << err;
    }
    wbuf.resize(bufLen);
    if (!GetComputerNameExW(
        ComputerNameDnsFullyQualified,
        wbuf.data(),
        &bufLen
    )) {
        WinError err;
        logMsgFatal() << "GetComputerNameExW: " << err;
    }
    return toString(wbuf);
}

//===========================================================================
EnvDomainMembership Dim::envDomainMembership() {
    EnvDomainMembership out = {};
    wchar_t * buf;
    NETSETUP_JOIN_STATUS bufType;
    if (WinError err = NetGetJoinInformation(NULL, &buf, &bufType))
        logMsgFatal() << "NetGetJoinInformation: " << err;
    switch (bufType) {
    case NetSetupUnknownStatus: out.status = DomainStatus::kUnknown; break;
    case NetSetupUnjoined: out.status = DomainStatus::kUnjoined; break;
    case NetSetupWorkgroupName: out.status = DomainStatus::kWorkgroup;break;
    case NetSetupDomainName: out.status = DomainStatus::kDomain; break;
    default:
        out.status = DomainStatus::kUnknown;
    }
    out.name = toString(buf);
    if (WinError err = NetApiBufferFree(buf))
        logMsgFatal() << "NetApiBufferFree: " << err;
    return out;
}

//===========================================================================
string Dim::envDomainStatusToString(DomainStatus value) {
    switch (value) {
    case DomainStatus::kUnknown: return "unknown";
    case DomainStatus::kUnjoined: return "unjoined";
    case DomainStatus::kWorkgroup: return "workgroup";
    case DomainStatus::kDomain: return "domain";
    }
    string out = "UNKNOWN(" + toString(value) + ')';
    return out;
}

//===========================================================================
Dim::VersionInfo Dim::envOSVersion() {
    VersionInfo out = {};
    if (!s_RtlGetVersion)
        winLoadProc(&s_RtlGetVersion, "ntdll", "RtlGetVersion");
    DIM_OSVERSIONINFOEXW info = { sizeof(info) };
    WinError err = s_RtlGetVersion(&info);
    if (!err) {
        out.major = info.dwMajorVersion;
        out.minor = info.dwMinorVersion;
        out.build = info.dwBuildNumber;
        out.patch = 0;
    }
    return out;
}

//===========================================================================
void Dim::envOSVersion(IJBuilder * out) {
    if (!s_RtlGetVersion)
        winLoadProc(&s_RtlGetVersion, "ntdll", "RtlGetVersion");
    DIM_OSVERSIONINFOEXW info = { sizeof(info) };
    if (WinError err = s_RtlGetVersion(&info)) {
        logMsgError() << "RtlGetVersion: " << err;
        return;
    }
    out->member("version").object();
    out->member("major", info.dwMajorVersion)
        .member("minor", info.dwMinorVersion)
        .member("build", info.dwBuildNumber)
        .member("svcPackName", toString(info.szCSDVersion))
        .member("svcPackMajor", info.wServicePackMajor)
        .member("svcPackMinor", info.wServicePackMinor);
    out->end();
}


/****************************************************************************
*
*   Executable of Current Process
*
***/

//===========================================================================
static string execPath() {
    // While you'd think you could use _get_wpgmptr() or _get_pgmptr() and save
    // a kernel call it's not so easy. They both assert that the program was
    // started via the matching flavor of wmain or main. A fact which,
    // depending on the app, might change based on being started as a service
    // or console application.
    DWORD num = MAX_PATH;
    wstring wpath(num, '\0');
    for (;;) {
        num = GetModuleFileNameW(
            NULL,
            wpath.data(),
            (DWORD) wpath.size()
        );
        if (!num)
            logMsgFatal() << "GetModuleFileNameW(NULL): " << WinError();
        if (num != wpath.size())
            break;
        wpath.resize(2 * wpath.size());
    }
    wpath.resize(num);
    return toString(wpath);
}

//===========================================================================
const string & Dim::envExecPath() {
    static string path = execPath();
    return path;
}

//===========================================================================
Dim::VersionInfo Dim::envExecVersion() {
    VersionInfo out = {};

    auto wname = toWstring(envExecPath());
    DWORD unused;
    auto len = GetFileVersionInfoSizeW(wname.c_str(), &unused);
    if (!len)
        return out;

    auto buf = make_unique<char[]>(len);
    if (!GetFileVersionInfoW(
        wname.c_str(),
        0,  // ignored
        len,
        buf.get()
    )) {
        logMsgFatal() << "GetFileVersionInfoW: " << WinError{};
    }
    VS_FIXEDFILEINFO * fi;
    UINT ulen;
    if (!VerQueryValueW(buf.get(), L"\\", (void **) &fi, &ulen))
        logMsgFatal() << "VerQueryValueW: " << WinError{};

    out.major = HIWORD(fi->dwProductVersionMS);
    out.minor = LOWORD(fi->dwProductVersionMS);
    out.patch = HIWORD(fi->dwProductVersionLS);
    out.build = LOWORD(fi->dwProductVersionLS);
    return out;
}

//===========================================================================
TimePoint Dim::envExecBuildTime() {
    // GetTimestampForLoadedLibrary is a DbgHelp function and therefore access
    // must be serialized.
    static auto time = []() {
        auto utime = GetTimestampForLoadedLibrary(GetModuleHandle(NULL));
        return timeFromUnix(utime);
    }();
    return time;
}


/****************************************************************************
*
*   Current Process
*
***/

//===========================================================================
unsigned Dim::envProcessId() {
    return GetCurrentProcessId();
}

//===========================================================================
TimePoint Dim::envProcessStartTime() {
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    if (!GetProcessTimes(
        GetCurrentProcess(),
        &creation,
        &exit,
        &kernel,
        &user
    )) {
        return {};
    }
    TimePoint time{duration(creation)};
    return time;
}

//===========================================================================
ProcessRights Dim::envProcessRights() {
    char sid[SECURITY_MAX_SID_SIZE];
    DWORD cb = sizeof sid;
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, sid, &cb)) {
        logMsgError() << "CreateWellKnownSid(Administrators): " << WinError{};
        return kEnvUserStandard;
    }
    BOOL isMember;
    if (!CheckTokenMembership(NULL, sid, &isMember)) {
        logMsgError() << "CheckTokenMembership(NULL): " << WinError{};
        return kEnvUserStandard;
    }
    if (isMember)
        return kEnvUserAdmin;

    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_QUERY, &token))
        return kEnvUserStandard;
    Finally fin([=]() { CloseHandle(token); });

    TOKEN_LINKED_TOKEN lt;
    if (!GetTokenInformation(
        token,
        TokenLinkedToken,
        &lt,
        sizeof lt,
        &cb
    )) {
        WinError err;
        if (err == ERROR_NO_SUCH_LOGON_SESSION
            || err == ERROR_PRIVILEGE_NOT_HELD
        ) {
            return kEnvUserStandard;
        }

        logMsgError() << "GetTokenInformation(TokenLinkedToken): " << err;
        return kEnvUserStandard;
    }

    if (!CheckTokenMembership(lt.LinkedToken, sid, &isMember)) {
        logMsgError() << "CheckTokenMembership(linked): " << WinError{};
        return kEnvUserStandard;
    }
    if (isMember)
        return kEnvUserRestrictedAdmin;

    return kEnvUserStandard;
}

//===========================================================================
string Dim::envProcessLogDataDir() {
    PWSTR kfPath;
    auto res = SHGetKnownFolderPath(
        FOLDERID_LocalAppData,
        KF_FLAG_CREATE,
        NULL,
        &kfPath
    );
    if (FAILED(res)) {
        WinError err((WinError::HResult) res);
        logMsgFatal() << "SHGetKnownFolderPath(LocalAppData): " << err;
    }
    auto out = toString(kfPath);
    CoTaskMemFree(kfPath);
    return out;
}


/****************************************************************************
*
*   Accounts
*
***/

const TokenTable::Token s_sidAttrs[] = {
    { (int) SE_GROUP_ENABLED,             "Enabled" },
    { (int) SE_GROUP_ENABLED_BY_DEFAULT,  "EnabledByDefault" },
    { (int) SE_GROUP_INTEGRITY,           "Integrity" },
    { (int) SE_GROUP_INTEGRITY_ENABLED,   "IntegrityEnabled" },
    { (int) SE_GROUP_LOGON_ID,            "LogonId" },
    { (int) SE_GROUP_MANDATORY,           "Mandatory" },
    { (int) SE_GROUP_OWNER,               "Owner" },
    { (int) SE_GROUP_RESOURCE,            "Resource" },
    { (int) SE_GROUP_USE_FOR_DENY_ONLY,   "UseForDenyOnly" },
};
const TokenTable s_sidAttrTbl(s_sidAttrs);

const TokenTable::Token s_sidTypes[] = {
    { SidTypeAlias,             "Alias" },
    { SidTypeComputer,          "Computer" },
    { SidTypeDeletedAccount,    "DeletedAccount" },
    { SidTypeDomain,            "Domain" },
    { SidTypeGroup,             "Group" },
    { SidTypeInvalid,           "Invalid" },
    { SidTypeLabel,             "Label" },
    { SidTypeLogonSession,      "LogonSession" },
    { SidTypeUnknown,           "Unknown" },
    { SidTypeUser,              "User" },
    { SidTypeWellKnownGroup,    "WellKnownGroup" },
};
const TokenTable s_sidTypeTbl(s_sidTypes);

const TokenTable::Token s_privAttrs[] = {
    { (int) SE_PRIVILEGE_ENABLED,
    "Enabled" },
    { (int) SE_PRIVILEGE_ENABLED_BY_DEFAULT, "EnabledByDefault" },
    { (int) SE_PRIVILEGE_USED_FOR_ACCESS,    "UsedForAccess" },
};
const TokenTable s_privAttrTbl(s_privAttrs);

//===========================================================================
static void addAttrs(IJBuilder * out, int attrs, const TokenTable & tbl) {
    out->array();
    unsigned found = {};
    for (auto && attr : tbl) {
        if (attr.id & attrs) {
            found |= attr.id;
            out->value(attr.name);
        }
    }
    if (auto unknown = ~found & attrs) {
        auto unk = "UNKNOWN(" + toString(unknown) + ')';
        out->value(unk);
    }
    out->end();
}

//===========================================================================
static void addSidRow(IJBuilder * out, SID_AND_ATTRIBUTES & sa) {
    // Get domain and account names
    DWORD nameLen = 0;
    DWORD domLen = 0;
    SID_NAME_USE use;
    if (!LookupAccountSidW(
        NULL,
        sa.Sid,
        NULL, &nameLen,
        NULL, &domLen,
        &use
    )) {
        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER)
            logMsgFatal() << "LookupAccountSidW(NULL): " << err;
    }
    nameLen += 1;
    domLen += 1;
    wstring wname(nameLen, 0);
    wstring wdom(domLen, 0);
    if (!LookupAccountSidW(
        NULL,
        sa.Sid,
        wname.data(), &nameLen,
        wdom.data(), &domLen,
        &use
    )) {
        logMsgFatal() << "LookupAccountSidW: " << WinError{};
    }
    wname.resize(nameLen);
    wdom.resize(domLen);

    // Get sid string
    LPWSTR wsidPtr;
    if (!ConvertSidToStringSidW(sa.Sid, &wsidPtr))
        logMsgFatal() << "ConvertSidToStringSidW: " << WinError{};
    auto wsid = toString(wsidPtr);
    LocalFree(wsidPtr);

    out->object();
    out->member("sid", wsid);
    out->member("attrs");
    addAttrs(out, sa.Attributes, s_sidAttrTbl);
    out->member("name", toString(wname));
    out->member("domain", toString(wdom));
    if (auto name = s_sidTypeTbl.findName(use)) {
        out->member("type", name);
    } else {
        auto unk = "UNKNOWN(" + toString(use) + ')';
        out->member("type", unk);
    }
    out->end();
}

//===========================================================================
template<typename T>
static unique_ptr<T> getTokenInfo(
    HANDLE token,
    TOKEN_INFORMATION_CLASS iclass,
    string_view logName
) {
    DWORD len;
    BOOL result = GetTokenInformation(token, iclass, NULL, 0, &len);
    WinError err;
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
        logMsgFatal() << "GetTokenInformation(" << logName << ", NULL): "
            << err;
    }
    auto info = unique_ptr<T>((T *) malloc(len));
    if (!GetTokenInformation(token, iclass, info.get(), len, &len)) {
        err.set();
        logMsgFatal() << "GetTokenInformation(" << logName << "): " << err;
    }
    return info;
}

//===========================================================================
static string privilegeName(LUID id) {
    DWORD len = 0;
    BOOL result = LookupPrivilegeNameA(NULL, &id, NULL, &len);
    WinError err;
    if (result || err != ERROR_INSUFFICIENT_BUFFER)
        logMsgFatal() << "LookupPrivilegeNameA(NULL): " << err;
    string out;
    out.resize(len);
    if (!LookupPrivilegeNameA(NULL, &id, out.data(), &len))
        logMsgFatal() << "LookupPrivilegeNameA: " << err.set();
    out.pop_back();
    return out;
}

//===========================================================================
void Dim::envProcessAccountInfo(IJBuilder * out) {
    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_QUERY, &token))
        logMsgFatal() << "OpenProcessToken: " << WinError{};

    auto usr = getTokenInfo<TOKEN_USER>(token, TokenUser, "TokenUser");
    auto grps = getTokenInfo<TOKEN_GROUPS_AND_PRIVILEGES>(
        token,
        TokenGroupsAndPrivileges,
        "TokenGroupsAndPrivileges"
    );
    CloseHandle(token);

    out->member("user");
    addSidRow(out, usr->User);
    out->member("groups").array();
    for (auto&& val : span(grps->Sids, grps->SidCount))
        addSidRow(out, val);
    out->end();
    out->member("restrictedGroups").array();
    for (auto&& val : span(grps->RestrictedSids, grps->RestrictedSidCount))
        addSidRow(out, val);
    out->end();
    out->member("privileges").array();
    for (auto&& val : span(grps->Privileges, grps->PrivilegeCount)) {
        out->object();
        out->member("attrs");
        addAttrs(out, val.Attributes, s_privAttrTbl);
        out->member("name", privilegeName(val.Luid));
        out->end();
    }
    out->end();
}

//===========================================================================
string Dim::envProcessAccount() {
    return s_procAcct;
}


/****************************************************************************
*
*   Environment Variables
*
***/

//===========================================================================
map<string, string> Dim::envGetVars() {
    map<string, string> out;
    auto wenv = GetEnvironmentStringsW();
    if (!wenv) {
        logMsgError() << "GetEnvironmentStringsW: " << WinError{};
        return out;
    }
    vector<string_view> nv;
    wstring_view wvar;
    for (auto wvars = wenv; *wvars; wvars += wvar.size() + 1) {
        wvar = wstring_view(wvars);
        auto var = toString(wvar);
        split(&nv, var, '=');
        if (nv.size() == 2 && !nv[0].empty() && !nv[1].empty())
            out[string(nv[0])] = nv[1];
    }
    if (!FreeEnvironmentStringsW(wenv))
        logMsgError() << "FreeEnvironmentStringsW: " << WinError{};
    return out;
}

//===========================================================================
string Dim::envGetVar(string_view name) {
    wstring wvar;
    wvar.resize(wvar.capacity());
    auto wname = toWstring(name);
    DWORD cnt = 0;
    for (;;) {
        cnt = GetEnvironmentVariableW(
            wname.data(),
            wvar.data(),
            (DWORD) wvar.size() + 1 // include space of terminating null
        );
        if (!cnt) {
            WinError err;
            if (err != ERROR_ENVVAR_NOT_FOUND) {
                logMsgError() << "GetEnvironmentVariableW(" << name << "): "
                    << err;
            }
            return {};
        }
        if (cnt <= wvar.size()) {
            wvar.resize(cnt);
            return toString(wvar);
        }
        wvar.resize(cnt - 1);
    }
}

//===========================================================================
bool Dim::envSetVar(string_view name, string_view value) {
    if (!SetEnvironmentVariableW(
        toWstring(name).c_str(),
        toWstring(value).c_str()
    )) {
        WinError err;
        logMsgError() << "SetEnvironmentVariableW(" << name << "): " << err;
        return false;
    }
    return true;
}
