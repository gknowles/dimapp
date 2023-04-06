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
    template<typename T> class Param;
    template<typename T> class ParamVec;

public:
    virtual ~IHttpRouteNotify() = default;
    virtual void onHttpRequest(unsigned reqId, HttpRequest & msg) = 0;

    void mapParams(const HttpRequest & msg);

private:
    std::vector<ParamBase *> m_params;
    TokenTable m_paramTbl;
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
*   Parsing request parameters
*
***/

class IHttpRouteNotify::ParamBase {
public:
    ParamBase(IHttpRouteNotify * owner, std::string name);
    ParamBase(const ParamBase &) = delete;
    virtual ~ParamBase() = default;
    ParamBase & operator=(const ParamBase &) = delete;
    virtual void reset() = 0;
    virtual void append(std::string_view value) = 0;
    virtual void finalize() = 0;
protected:
    friend IHttpRouteNotify;
    std::string m_name;
    bool m_hasDefault {false};
};

template<typename T = std::string_view>
class IHttpRouteNotify::Param : public ParamBase {
public:
    using ParamBase::ParamBase;
    Param(IHttpRouteNotify * owner, std::string name, const T & defVal);
    explicit operator bool() const { return m_explicit; }
    T & operator*() { return m_value; }
    T * operator->() { return &m_value; }
private:
    void reset() final;
    void append(std::string_view value) final;
    void finalize() final;

    T m_value;
    bool m_explicit {false};
    T m_default;
};

template<typename T = std::string_view>
class IHttpRouteNotify::ParamVec : public ParamBase {
public:
    using ParamBase::ParamBase;
    explicit operator bool() const { return !m_values.empty(); }
    std::vector<T> & operator*() { return m_values; }
    std::vector<T> * operator->() { return &m_values; }
    T & operator[](size_t index) { return m_values[index]; }
    size_t size() const { return m_values.size(); }
private:
    void reset() final;
    void append(std::string_view value) final;
    void finalize() final;

    std::vector<T> m_values;
    std::vector<T> m_default;
};

//===========================================================================
// IHttpRouteNotify::ParamBase
//===========================================================================
inline IHttpRouteNotify::ParamBase::ParamBase(
    IHttpRouteNotify * owner,
    std::string name
)
    : m_name{move(name)}
{
    owner->m_params.push_back(this);
}

//===========================================================================
// IHttpRouteNotify::Param<T>
//===========================================================================
template<typename T>
inline IHttpRouteNotify::Param<T>::Param(
    IHttpRouteNotify * owner,
    std::string name,
    const T & defVal
)
    : ParamBase{owner, move(name)}
{
    m_default = defVal;
    m_hasDefault = true;
}

//===========================================================================
template<typename T>
inline void IHttpRouteNotify::Param<T>::append(std::string_view value) {
    if (parse(&m_value, value))
        m_explicit = true;
}

//===========================================================================
template<typename T>
inline void IHttpRouteNotify::Param<T>::finalize() {
    if (!m_explicit && m_hasDefault)
        m_value = m_default;
}

//===========================================================================
template<typename T>
inline void IHttpRouteNotify::Param<T>::reset() {
    m_value = {};
    m_explicit = false;
}

//===========================================================================
// IHttpRouteNotify::ParamVec<T>
//===========================================================================
template<typename T>
inline void IHttpRouteNotify::ParamVec<T>::append(std::string_view value) {
    T val;
    if (parse(&val, value))
        m_values.push_back(val);
}

//===========================================================================
template<typename T>
inline void IHttpRouteNotify::ParamVec<T>::finalize() {
    if (m_values.empty() && m_hasDefault)
        m_values = m_default;
}

//===========================================================================
template<typename T>
inline void IHttpRouteNotify::ParamVec<T>::reset() {
    m_values.clear();
}


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
