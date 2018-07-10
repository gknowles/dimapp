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
    CharBuf(const CharBuf & from) { insert(0, from); }
    CharBuf(CharBuf && from) = default;
    ~CharBuf();
    explicit operator bool() const { return !empty(); }

    CharBuf & operator=(const CharBuf & buf) { return assign(buf); }
    CharBuf & operator=(CharBuf && buf) = default;
    CharBuf & operator=(char ch) { return assign(ch); }
    CharBuf & operator=(const char s[]) { return assign(s); }
    CharBuf & operator=(std::string_view str) { return assign(str); }
    CharBuf & operator+=(char ch) { pushBack(ch); return *this; }
    CharBuf & operator+=(const char s[]) { return append(s); }
    CharBuf & operator+=(std::string_view str) { return append(str); }
    CharBuf & operator+=(const CharBuf & src) { return append(src); }
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
    int compare(const char s[], size_t count) const;
    int compare(size_t pos, size_t count, const char s[], size_t slen) const;
    int compare(std::string_view str) const;
    int compare(size_t pos, size_t count, std::string_view str) const;
    int compare(const CharBuf & buf, size_t pos = 0, size_t count = -1) const;
    int compare(
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

    // ITempHeap
    char * alloc(size_t bytes, size_t align) override;

private:
    struct Buffer : public NoCopy {
        char * data;
        int used{0};
        int reserved;
        bool heapUsed{false};

        Buffer();
        Buffer(size_t reserve);
        Buffer(Buffer && from);
        ~Buffer();
        Buffer & operator=(Buffer && from);
    };
    using buffer_iterator = std::vector<Buffer>::iterator;
    using const_buffer_iterator = std::vector<Buffer>::const_iterator;

    std::pair<buffer_iterator, int> find(size_t pos);
    std::pair<const_buffer_iterator, int> find(size_t pos) const;
    buffer_iterator split(buffer_iterator it, int pos);
    CharBuf & insert(buffer_iterator it, int pos, size_t numCh, char ch);
    CharBuf & insert(buffer_iterator it, int pos, const char src[]);
    CharBuf & insert(
        buffer_iterator it,
        int pos,
        const char src[],
        size_t srcLen
    );
    CharBuf & insert(
        buffer_iterator it,
        int pos,
        const_buffer_iterator srcIt,
        int srcPos,
        size_t srcLen
    );
    CharBuf & erase(buffer_iterator it, int pos, int count);

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
    bool operator!= (const ViewIterator & right);
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


/****************************************************************************
*
*   Free functions
*
***/

bool operator==(const CharBuf & left, std::string_view right);
bool operator==(std::string_view left, const CharBuf & right);
bool operator==(const CharBuf & left, const CharBuf & right);

std::string toString(const CharBuf & buf, size_t pos = 0, size_t count = -1);

std::ostream & operator<<(std::ostream & os, const CharBuf & buf);

} // namespace
