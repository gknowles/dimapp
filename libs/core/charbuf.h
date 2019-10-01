// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// charbuf.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/tempheap.h"
#include "core/types.h"

#include <algorithm>
#include <cassert>
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

class CharBuf : public ITempHeap {
public:
    class ViewIterator;

public:
    CharBuf() {}
    CharBuf(CharBuf const & from) { insert(0, from); }
    CharBuf(CharBuf && from) noexcept = default;
    ~CharBuf();
    explicit operator bool() const { return !empty(); }

    CharBuf & operator=(CharBuf const & buf) { return assign(buf); }
    CharBuf & operator=(CharBuf && buf) = default;
    CharBuf & operator=(char ch) { return assign(ch); }
    CharBuf & operator=(char const s[]) { return assign(s); }
    CharBuf & operator=(std::string_view str) { return assign(str); }
    CharBuf & operator+=(char ch) { pushBack(ch); return *this; }
    CharBuf & operator+=(char const s[]) { return append(s); }
    CharBuf & operator+=(std::string_view str) { return append(str); }
    CharBuf & operator+=(CharBuf const & src) { return append(src); }
    CharBuf & assign(char ch) { clear(); pushBack(ch); return *this; }
    CharBuf & assign(size_t numCh, char ch);
    CharBuf & assign(std::nullptr_t);
    CharBuf & assign(char const s[]);
    CharBuf & assign(char const s[], size_t slen);
    CharBuf & assign(std::string_view str, size_t pos = 0, size_t count = -1);
    CharBuf & assign(CharBuf const & buf, size_t pos = 0, size_t count = -1);
    char & front();
    char const & front() const;
    char & back();
    char const & back() const;
    bool empty() const;
    size_t size() const;
    char const * data() const;
    char const * data(size_t pos, size_t count = -1) const;
    char * data();
    char * data(size_t pos, size_t count = -1);
    char const * c_str() const;
    char * c_str();
    std::string_view view() const;
    std::string_view view(size_t pos, size_t count = -1) const;
    ViewIterator views(size_t pos = 0, size_t count = -1) const;
    void clear();
    void resize(size_t count);
    CharBuf & insert(size_t pos, size_t numCh, char ch);
    CharBuf & insert(size_t pos, std::nullptr_t);
    CharBuf & insert(size_t pos, char const s[]);
    CharBuf & insert(size_t pos, char const s[], size_t count);
    CharBuf & insert(
        size_t pos,
        CharBuf const & buf,
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
    CharBuf & append(char const s[]);
    CharBuf & append(char const s[], size_t slen);
    CharBuf & append(std::string_view str, size_t pos = 0, size_t count = -1);
    CharBuf & append(CharBuf const & buf, size_t pos = 0, size_t count = -1);
    int compare(char const s[], size_t count) const;
    int compare(size_t pos, size_t count, char const s[], size_t slen) const;
    int compare(std::string_view str) const;
    int compare(size_t pos, size_t count, std::string_view str) const;
    int compare(CharBuf const & buf, size_t pos = 0, size_t count = -1) const;
    int compare(
        size_t pos,
        size_t count,
        CharBuf const & buf,
        size_t bufPos = 0,
        size_t bufLen = -1
    ) const;
    CharBuf & replace(size_t pos, size_t count, size_t numCh, char ch);
    CharBuf & replace(size_t pos, size_t count, std::nullptr_t);
    CharBuf & replace(size_t pos, size_t count, char const s[]);
    CharBuf & replace(size_t pos, size_t count, char const s[], size_t slen);
    CharBuf & replace(
        size_t pos,
        size_t count,
        CharBuf const & src,
        size_t srcPos = 0,
        size_t srcLen = -1
    );
    size_t copy(char * out, size_t count, size_t pos = 0) const;
    void swap(CharBuf & other);

    size_t defaultBlockSize() const;

    // ITempHeap
    char * alloc(size_t bytes, size_t align) override;

private:
    friend bool operator==(CharBuf const & left, std::string_view right);
    friend bool operator==(std::string_view left, CharBuf const & right);
    friend bool operator==(CharBuf const & left, CharBuf const & right);

    friend std::string toString(
        CharBuf const & buf,
        size_t pos = 0,
        size_t count = -1
    );

    friend std::ostream & operator<<(std::ostream & os, CharBuf const & buf);

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
    CharBuf & insert(buffer_iterator it, size_t pos, char const src[]);
    CharBuf & insert(
        buffer_iterator it,
        size_t pos,
        char const src[],
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
    int m_lastUsed{0};
    int m_size{0};
    char m_empty{0};
};


/****************************************************************************
*
*   CharBuf::ViewIterator
*
***/

class CharBuf::ViewIterator {
    const_buffer_iterator m_current;
    std::string_view m_view;
    size_t m_count{0};
public:
    ViewIterator() {}
    ViewIterator(
        const_buffer_iterator buf,
        size_t pos,
        size_t count
    );
    bool operator!= (ViewIterator const & right);
    ViewIterator & operator++();
    std::string_view const & operator*() const { return m_view; }
    std::string_view const * operator->() const { return &m_view; }
};

//===========================================================================
// Free
inline CharBuf::ViewIterator begin (CharBuf::ViewIterator iter) {
    return iter;
}

//===========================================================================
// Free
inline CharBuf::ViewIterator end (CharBuf::ViewIterator const & iter) {
    return {};
}


/****************************************************************************
*
*   Free functions
*
***/


} // namespace
