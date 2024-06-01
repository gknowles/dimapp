// Copyright Glen Knowles 2015 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// charbuf.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <algorithm>
#include <cassert>
#include <compare>
#include <list>
#include <string>
#include <string_view>
#include <utility> // std::pair
#include <vector>

namespace Dim {


/****************************************************************************
*
*   CharBuf
*
***/

class CharBuf {
public:
    class ViewIterator;

public:
    CharBuf() {}
    CharBuf(const CharBuf & from) { insert(0, from); }
    CharBuf(CharBuf && from) noexcept;
    explicit operator bool() const { return !empty(); }

    CharBuf & operator=(const CharBuf & buf) { return assign(buf); }
    CharBuf & operator=(CharBuf && buf) noexcept;
    CharBuf & operator=(char ch) { return assign(ch); }
    CharBuf & operator=(const char s[]) { return assign(s); }
    CharBuf & operator=(std::string_view str) { return assign(str); }
    CharBuf & operator+=(char ch) { pushBack(ch); return *this; }
    CharBuf & operator+=(const char s[]) { return append(s); }
    CharBuf & operator+=(std::string_view str) { return append(str); }
    CharBuf & operator+=(const CharBuf & src) { return append(src); }

    std::strong_ordering operator<=>(const CharBuf & right) const {
        return compare(right);
    }
    std::strong_ordering operator<=>(std::string_view right) const {
        return compare(right);
    }
    std::strong_ordering operator<=>(const char right[]) const {
        return compare(right);
    }

    bool operator==(const CharBuf & right) const {
        return compare(right) == 0;
    }
    bool operator==(std::string_view right) const {
        return compare(right) == 0;
    }
    bool operator==(const char right[]) const { return compare(right) == 0; }

    CharBuf & assign(char ch) { clear(); pushBack(ch); return *this; }
    CharBuf & assign(size_t numCh, char ch);
    CharBuf & assign(std::nullptr_t);
    CharBuf & assign(const char s[]);
    CharBuf & assign(const char s[], size_t slen);
    CharBuf & assign(std::string_view str, size_t pos = 0, size_t count = -1);
    CharBuf & assign(const CharBuf & buf, size_t pos = 0, size_t count = -1);
    char & front();
    const char & front() const;
    char & back();
    const char & back() const;
    bool empty() const;
    size_t size() const;
    const char * data() const;
    const char * data(size_t pos, size_t count = -1) const;
    char * data();
    char * data(size_t pos, size_t count = -1);
    const char * c_str() const;
    char * c_str();
    std::string_view view() const;
    std::string_view view(size_t pos, size_t count = -1) const;
    ViewIterator views(size_t pos = 0, size_t count = -1) const;
    void clear();
    void resize(size_t count);
    CharBuf & insert(size_t pos, size_t numCh, char ch);
    CharBuf & insert(size_t pos, std::nullptr_t);
    CharBuf & insert(size_t pos, const char s[]);
    CharBuf & insert(size_t pos, const char s[], size_t count);
    CharBuf & insert(
        size_t pos,
        std::string_view str,
        size_t strPos,
        size_t strCount
    );
    CharBuf & insert(
        size_t pos,
        const CharBuf & buf,
        size_t bufPos = 0,
        size_t bufLen = -1
    );
    CharBuf & erase(size_t pos = 0, size_t count = -1);
    CharBuf & ltrim(char ch);
    CharBuf & rtrim(char ch);
    CharBuf & pushBack(char ch);
    CharBuf & popBack();
    CharBuf & append(size_t numCh, char ch);
    CharBuf & append(std::nullptr_t);
    CharBuf & append(const char s[]);
    CharBuf & append(const char s[], size_t slen);
    CharBuf & append(std::string_view str, size_t pos = 0, size_t count = -1);
    CharBuf & append(const CharBuf & buf, size_t pos = 0, size_t count = -1);
    std::strong_ordering compare(const char s[], size_t count) const;
    std::strong_ordering compare(
        size_t pos,
        size_t count,
        const char s[],
        size_t slen
    ) const;
    std::strong_ordering compare(std::string_view str) const;
    std::strong_ordering compare(
        size_t pos,
        size_t count,
        std::string_view str
    ) const;
    std::strong_ordering compare(
        const CharBuf & buf,
        size_t pos = 0,
        size_t count = -1
    ) const;
    std::strong_ordering compare(
        size_t pos,
        size_t count,
        const CharBuf & buf,
        size_t bufPos = 0,
        size_t bufLen = -1
    ) const;
    CharBuf & replace(size_t pos, size_t count, size_t numCh, char ch);
    CharBuf & replace(size_t pos, size_t count, std::nullptr_t);
    CharBuf & replace(size_t pos, size_t count, const char s[]);
    CharBuf & replace(size_t pos, size_t count, const char s[], size_t slen);
    CharBuf & replace(
        size_t pos,
        size_t count,
        const CharBuf & src,
        size_t srcPos = 0,
        size_t srcLen = -1
    );
    size_t copy(char * out, size_t count, size_t pos = 0) const;
    void swap(CharBuf & other);

    size_t defaultBlockSize() const;

private:
    friend std::string toString(const CharBuf & buf);
    friend std::string toString(const CharBuf & buf, size_t pos);
    friend std::string toString(const CharBuf & buf, size_t pos, size_t count);

    friend std::ostream & operator<<(std::ostream & os, const CharBuf & buf);

private:
    struct Buffer : public NoCopy {
        char * data;
        int used{0};
        int reserved;
        bool heapUsed{false};

        Buffer();
        Buffer(size_t reserve);
        Buffer(Buffer && from) noexcept;
        ~Buffer();
        Buffer & operator=(Buffer && from) noexcept;
    };
    using buffer_iterator = std::vector<Buffer>::iterator;
    using const_buffer_iterator = std::vector<Buffer>::const_iterator;

    std::pair<buffer_iterator, int> find(size_t pos);
    std::pair<const_buffer_iterator, int> find(size_t pos) const;
    buffer_iterator split(buffer_iterator it, size_t pos);
    CharBuf & insert(buffer_iterator it, size_t pos, size_t numCh, char ch);
    CharBuf & insert(buffer_iterator it, size_t pos, const char src[]);
    CharBuf & insert(
        buffer_iterator it,
        size_t pos,
        const char src[],
        size_t srcLen
    );
    CharBuf & insert(
        buffer_iterator it,
        size_t pos,
        const_buffer_iterator srcIt,
        size_t srcPos,
        size_t srcLen
    );
    CharBuf & erase(buffer_iterator it, size_t pos, size_t count);

    std::vector<Buffer> m_buffers;
    int m_lastUsed {0};
    int m_size {0};
    char m_empty {0};
};


/****************************************************************************
*
*   CharBuf::ViewIterator
*
***/

class CharBuf::ViewIterator {
    static_assert(std::is_same_v<
        decltype(CharBuf::m_buffers)::const_iterator::iterator_concept,
        std::contiguous_iterator_tag>);
    const Buffer * m_current = {};
    std::string_view m_view;
    size_t m_count = {};
public:
    ViewIterator() {}
    ViewIterator(
        const_buffer_iterator buf,
        size_t pos,
        size_t count
    );
    bool operator== (const ViewIterator & right) const;
    ViewIterator & operator++();
    const std::string_view & operator*() const { return m_view; }
    const std::string_view * operator->() const { return &m_view; }
};

//===========================================================================
// Free
inline CharBuf::ViewIterator begin (CharBuf::ViewIterator iter) {
    return iter;
}

//===========================================================================
// Free
inline CharBuf::ViewIterator end (const CharBuf::ViewIterator & iter) {
    return {};
}


} // namespace
