// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// hpack.h - dim http
//
// implements hpack, as defined by:
//  rfc7541 - HPACK: Header Compression for HTTP/2
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "net/http.h"

#include <deque>
#include <string>

namespace Dim {


/****************************************************************************
*
*   Common
*
***/

struct HpackFieldView;
enum HpackFlags : unsigned {
    fNeverIndexed = 1,
};

struct HpackDynField {
    std::string name;
    std::string value;
};


/****************************************************************************
*
*   HpackEncode
*
***/

class HpackEncode {
public:
    HpackEncode(size_t tableSize);
    void setTableSize(size_t tableSize);

    void startBlock(CharBuf * out);
    void endBlock();

    void header(
        char const name[],
        char const value[],
        HpackFlags flags = {}
    );
    void header(
        HttpHdr name,
        char const value[],
        HpackFlags flags = {}
    );

private:
    void write(char const str[]);
    void write(char const str[], size_t len);
    void write(size_t val, char prefix, int prefixBits);

    size_t m_dynSize{};
    CharBuf * m_out{};
};


/****************************************************************************
*
*   HpackDecode
*
***/

class IHpackDecodeNotify {
public:
    virtual ~IHpackDecodeNotify() = default;

    virtual void onHpackHeader(
        HttpHdr id,
        char const name[],
        char const value[],
        HpackFlags flags
    ) = 0;
};

class HpackDecode {
public:
    HpackDecode(size_t tableSize);
    void reset();
    void setTableSize(size_t tableSize);

    [[nodiscard]] bool parse(
        IHpackDecodeNotify * notify,
        ITempHeap * heap,
        char const src[],
        size_t srcLen
    );
    auto & dynamicTable() const { return m_dynTable; }

private:
    void pruneDynTable();

    bool readInstruction(
        IHpackDecodeNotify * notify,
        ITempHeap * heap,
        char const *& src,
        size_t & srcLen
    );
    bool readIndexedField(
        HpackFieldView * out,
        ITempHeap * heap,
        size_t prefixBits,
        char const *& src,
        size_t & srcLen
    );
    bool readIndexedName(
        HpackFieldView * out,
        ITempHeap * heap,
        size_t prefixBits,
        char const *& src,
        size_t & srcLen
    );
    bool read(
        size_t * out,
        size_t prefixBits,
        char const *& src,
        size_t & srcLen
    );
    bool read(
        char const ** out,
        ITempHeap * heap,
        char const *& src,
        size_t & srcLen
    );

    size_t m_dynSize{0};
    std::deque<HpackDynField> m_dynTable;
    size_t m_dynUsed{0};
};

} // namespace
