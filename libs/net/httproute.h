// Copyright Glen Knowles 2017 - 2018.
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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Configuring routes
*
***/

class IHttpRouteNotify {
public:
    class ParamBase {
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
    class Param : public ParamBase {
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
    class ParamVec : public ParamBase {
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

void httpRouteAdd(
    IHttpRouteNotify * notify,
    std::string_view path,
    HttpMethod methods = fHttpMethodGet,
    bool recurse = false
);
void httpRouteAddRedirect(
    std::string_view path,
    std::string_view location,
    unsigned status = 303,
    std::string_view msg = {}
);
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


/****************************************************************************
*
*   Query information about requests
*
***/

struct HttpRouteInfo {
    std::string_view path;
    HttpMethod methods;
    bool recurse;

    explicit operator bool() const { return methods != fHttpMethodInvalid; }
};
HttpRouteInfo httpRouteGetInfo(unsigned reqId);


/****************************************************************************
*
*   Replying to requests
*
***/

void httpRouteReply(unsigned reqId, HttpResponse && msg, bool more = false);
void httpRouteReply(unsigned reqId, CharBuf && data, bool more);
void httpRouteReply(unsigned reqId, std::string_view data, bool more);

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
void httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    std::string_view msg = {}
);
void httpRouteReply(unsigned reqId, unsigned status, std::string_view msg = {});


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
};
int compareExt(const MimeType & a, const MimeType & b);
MimeType mimeTypeDefault(std::string_view path);


/****************************************************************************
*
*   Debugging
*
***/

void httpRouteGetRoutes(IJBuilder * out);

} // namespace
