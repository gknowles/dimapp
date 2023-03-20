// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// httpRoute.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/tokentable.h"
#include "file/file.h"
#include "json/json.h"
#include "net/http.h"
#include "net/url.h"

#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Dim {

class IHttpRouteNotify;


/****************************************************************************
*
*   Declarations
*
***/

struct HttpRouteInfo {
    // Main settings.
    IHttpRouteNotify * notify = {};

    // Match conditions.
    std::string_view path;
    Dim::EnumFlags<HttpMethod> methods = fHttpMethodGet;
    bool recurse = false;

    // Web console parameters. IWebAdminNotify only reports routes with
    // non-empty names.
    std::string_view name;
    std::string_view desc;
    std::string_view renderPath;

    // Values not set by clients.
    unsigned matched = 0;
    Dim::TimePoint lastMatched = {};

    explicit operator bool() const { return methods.any(); }
};


/****************************************************************************
*
*   Configuring routes
*
***/

class IHttpRouteNotify {
public:
    class ParamBase;
    class Param;
    class ParamVec;

public:
    virtual ~IHttpRouteNotify() = default;
    virtual void onHttpRequest(unsigned reqId, HttpRequest & msg) = 0;

    void mapParams(const HttpRequest & msg);

protected:
    Param & param(std::string name);
    ParamVec & paramVec(std::string name);

private:
    std::vector<std::unique_ptr<ParamBase>> m_params;
    TokenTable m_paramTbl;
};

class IHttpRouteNotify::ParamBase {
public:
    explicit ParamBase(std::string name) : m_name{move(name)} {}
    virtual ~ParamBase() = default;
    ParamBase(const ParamBase &) = delete;
    ParamBase & operator=(const ParamBase &) = delete;
    virtual void append(std::string_view value) = 0;
    virtual void reset() = 0;
protected:
    friend IHttpRouteNotify;
    std::string m_name;
};

class IHttpRouteNotify::Param : public ParamBase {
public:
    using ParamBase::ParamBase;
    std::string_view & operator*() { return m_value; }
    std::string_view * operator->() { return &m_value; }
    explicit operator bool() const { return m_explicit; }
private:
    void append(std::string_view value) final {
        m_value = value;
        m_explicit = true;
    }
    void reset() final { m_value = {}; m_explicit = false; }
    std::string_view m_value;
    bool m_explicit {false};
};

class IHttpRouteNotify::ParamVec : public ParamBase {
public:
    using ParamBase::ParamBase;
    std::vector<std::string_view> & operator*() { return m_values; }
    std::vector<std::string_view> * operator->() { return &m_values; }
    std::string_view & operator[](size_t index) { return m_values[index]; }
    explicit operator bool() const { return !m_values.empty(); }
private:
    void append(std::string_view value) final { m_values.push_back(value); }
    void reset() final { m_values.clear(); }
    std::vector<std::string_view> m_values;
};

class HttpRouteRedirectNotify : public IHttpRouteNotify {
public:
    HttpRouteRedirectNotify(
        std::string_view location,
        unsigned status = 303,
        std::string_view msg = {}
    );
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
private:
    std::string m_location;
    unsigned m_status {};
    std::string m_msg;
};

class HttpRouteDirListNotify : public IHttpRouteNotify {
public:
    HttpRouteDirListNotify(std::string_view path);
    HttpRouteDirListNotify & set(std::string_view path);
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
private:
    std::string m_path;
};

void httpRouteAdd(const HttpRouteInfo & route);
void httpRouteAdd(std::initializer_list<HttpRouteInfo> routes);
void httpRouteAdd(std::span<const HttpRouteInfo> routes);

void httpRouteAddFile(
    std::string_view path,
    TimePoint mtime,
    std::string_view content,
    std::string_view mimeType = {},
    std::string_view charSet = {}
);
void httpRouteAddFileRef(
    std::string_view path,
    TimePoint mtime,
    std::string_view content,
    std::string_view mimeType = {},
    std::string_view charSet = {}
);
void httpRouteAddAlias(
    const HttpRouteInfo & alias,
    std::string_view targetPath,
    HttpMethod targetMethod = fHttpMethodGet
);


/****************************************************************************
*
*   Query information about requests
*
***/

HttpRouteInfo httpRouteGetInfo(unsigned reqId);


/****************************************************************************
*
*   Replying to requests
*
***/

void httpRouteReply(unsigned reqId, HttpResponse && msg, bool more = false);
void httpRouteReply(unsigned reqId, CharBuf && data, bool more);
void httpRouteReply(unsigned reqId, std::string_view data, bool more);

void httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    std::string_view msg = {}
);
void httpRouteReply(
    unsigned reqId,
    unsigned status,
    std::string_view msg = {}
);

// Aborts incomplete reply with CANCEL or INTERNAL_ERROR
void httpRouteCancel(unsigned reqId);
void httpRouteInternalError(unsigned reqId);

void httpRouteReplyWithFile(unsigned reqId, std::string_view path);
void httpRouteReplyWithFile(unsigned reqId, Dim::FileHandle file);
void httpRouteReplyWithFile(
    unsigned reqId,
    TimePoint mtime,
    std::string_view content,
    std::string_view mimeType,
    std::string_view charSet
);

void httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req);
void httpRouteReplyRedirect(
    unsigned reqId,
    std::string_view location,
    unsigned status = 303,
    std::string_view msg = {}
);
void httpRouteReplyDirList(
    unsigned reqId,
    const HttpRequest & req,
    std::string_view path
);


/****************************************************************************
*
*   Global settings
*
***/

void httpRouteSetDefaultReplyHeader(
    HttpHdr hdr,
    const char value[] = nullptr
);
void httpRouteSetDefaultReplyHeader(
    const char name[],
    const char value[] = nullptr
);


/****************************************************************************
*
*   Utilities
*
***/

struct MimeType {
    std::string_view fileExt;
    std::string_view type;
    std::string_view charSet;

    std::strong_ordering operator<=>(const MimeType & other) const;
    bool operator==(const MimeType & other) const = default;
};
MimeType mimeTypeDefault(std::string_view path);

std::vector<HttpRouteInfo> httpRouteGetRoutes();
void httpRouteWrite(
    IJBuilder * out,
    const HttpRouteInfo & ri,
    bool active = false
);

} // namespace
