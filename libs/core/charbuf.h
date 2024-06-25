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
#include <span>
#include <string>
#include <string_view>
#include <utility> // std::pair

namespace Dim {


/****************************************************************************
*
*   CharBufBase
*
***/

class CharBufBase {
public:
    class ViewIterator;

public:
    CharBufBase() {}
    virtual ~CharBufBase();
    explicit operator bool() const { return !empty(); }

    CharBufBase & operator=(const CharBufBase & buf) { return assign(buf); }
    CharBufBase & operator=(CharBufBase && buf) noexcept;
    CharBufBase & operator=(char ch) { return assign(ch); }
    CharBufBase & operator=(const char s[]) { return assign(s); }
    CharBufBase & operator=(std::string_view str) { return assign(str); }
    CharBufBase & operator+=(char ch) { return pushBack(ch); }
    CharBufBase & operator+=(const char s[]) { return append(s); }
    CharBufBase & operator+=(std::string_view str) { return append(str); }
    CharBufBase & operator+=(const CharBufBase & src) { return append(src); }

    std::strong_ordering operator<=>(const CharBufBase & right) const {
        return compare(right);
    }
    std::strong_ordering operator<=>(std::string_view right) const {
        return compare(right);
    }
    std::strong_ordering operator<=>(const char right[]) const {
        return compare(right);
    }

    bool operator==(const CharBufBase & right) const {
        return compare(right) == 0;
    }
    bool operator==(std::string_view right) const {
        return compare(right) == 0;
    }
    bool operator==(const char right[]) const { return compare(right) == 0; }

    CharBufBase & assign(char ch) { clear(); return pushBack(ch); }
    CharBufBase & assign(size_t numCh, char ch);
    CharBufBase & assign(std::nullptr_t);
    CharBufBase & assign(const char s[]);
    CharBufBase & assign(const char s[], size_t slen);
    CharBufBase & assign(
        std::string_view str,
        size_t pos = 0,
        size_t count = -1
    );
    CharBufBase & assign(
        const CharBufBase & buf,
        size_t pos = 0,
        size_t count = -1
    );
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
    CharBufBase & insert(size_t pos, size_t numCh, char ch);
    CharBufBase & insert(size_t pos, std::nullptr_t);
    CharBufBase & insert(size_t pos, const char s[]);
    CharBufBase & insert(size_t pos, const char s[], size_t count);
    CharBufBase & insert(
        size_t pos,
        std::string_view str,
        size_t strPos,
        size_t strCount
    );
    CharBufBase & insert(
        size_t pos,
        const CharBufBase & buf,
        size_t bufPos = 0,
        size_t bufLen = -1
    );
    CharBufBase & erase(size_t pos = 0, size_t count = -1);
    CharBufBase & ltrim(char ch);
    CharBufBase & rtrim(char ch);
    CharBufBase & pushBack(char ch);
    CharBufBase & popBack();
    CharBufBase & append(char ch) { return pushBack(ch); }
    CharBufBase & append(size_t numCh, char ch);
    CharBufBase & append(std::nullptr_t);
    CharBufBase & append(const char s[]);
    CharBufBase & append(const char s[], size_t slen);
    CharBufBase & append(
        std::string_view str,
        size_t pos = 0,
        size_t count = -1
    );
    CharBufBase & append(
        const CharBufBase & buf,
        size_t pos = 0,
        size_t count = -1
    );
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
        const CharBufBase & buf,
        size_t pos = 0,
        size_t count = -1
    ) const;
    std::strong_ordering compare(
        size_t pos,
        size_t count,
        const CharBufBase & buf,
        size_t bufPos = 0,
        size_t bufLen = -1
    ) const;
    CharBufBase & replace(size_t pos, size_t count, size_t numCh, char ch);
    CharBufBase & replace(size_t pos, size_t count, std::nullptr_t);
    CharBufBase & replace(size_t pos, size_t count, const char s[]);
    CharBufBase & replace(
        size_t pos,
        size_t count,
        const char s[],
        size_t slen
    );
    CharBufBase & replace(
        size_t pos,
        size_t count,
        const CharBufBase & src,
        size_t srcPos = 0,
        size_t srcLen = -1
    );
    size_t copy(char * out, size_t count, size_t pos = 0) const;
    void swap(CharBufBase & other);

    size_t defaultBlockSize() const;

private:
    friend std::string toString(const CharBufBase & buf);
    friend std::string toString(const CharBufBase & buf, size_t pos);
    friend std::string toString(
        const CharBufBase & buf,
        size_t pos,
        size_t count
    );

    friend std::ostream & operator<<(
        std::ostream & os,
        const CharBufBase & buf
    );

protected:
    void destruct();

    struct Buffer {
        char * data {nullptr};
        int used {0};
        int reserved {0};
        bool mustFree {false};
    };
    using buffer_iterator = Buffer *;
    using const_buffer_iterator = const Buffer *;

    std::pair<buffer_iterator, int> find(size_t pos);
    std::pair<const_buffer_iterator, int> find(size_t pos) const;
    buffer_iterator split(buffer_iterator it, size_t pos);
    CharBufBase & insert(
        buffer_iterator it,
        size_t pos,
        size_t numCh,
        char ch
    );
    CharBufBase & insert(buffer_iterator it, size_t pos, const char src[]);
    CharBufBase & insert(
        buffer_iterator it,
        size_t pos,
        const char src[],
        size_t srcLen
    );
    CharBufBase & insert(
        buffer_iterator it,
        size_t pos,
        const_buffer_iterator srcIt,
        size_t srcPos,
        size_t srcLen
    );
    CharBufBase & erase(buffer_iterator it, size_t pos, size_t count);

    buffer_iterator eraseBuf(buffer_iterator first, buffer_iterator last);
    Buffer makeBuf(size_t bytes);
    buffer_iterator appendBuf();
    buffer_iterator insertBuf(buffer_iterator pos);

    virtual void * allocate(size_t bytes);
    virtual void deallocate(void * ptr, size_t bytes);

    int m_size {0};
    char m_empty = '\0';    // empty string representation

    // buffers
    std::span<Buffer> m_buffers;
    size_t m_reserved = 0;
};


/****************************************************************************
*
*   CharBufBase::ViewIterator
*
***/

class CharBufBase::ViewIterator {
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
inline CharBufBase::ViewIterator begin (CharBufBase::ViewIterator iter) {
    return iter;
}

//===========================================================================
// Free
inline CharBufBase::ViewIterator end (const CharBufBase::ViewIterator & iter) {
    return {};
}


/****************************************************************************
*
*   CharBufAlloc
*
***/

template <typename A>
concept CharAllocator = (std::is_same_v<typename A::value_type, char>);

template <CharAllocator A = std::allocator<char>>
class CharBufAlloc : public CharBufBase {
public:
    using allocator_type = A;
    using alloc_traits = std::allocator_traits<allocator_type>;
    using buffer_alloc = alloc_traits::template rebind_alloc<Buffer>;

public:
    CharBufAlloc() {}
    CharBufAlloc(const CharBufAlloc & from) { insert(0, from); }
    CharBufAlloc(CharBufAlloc && from) noexcept { swap(from); }
    ~CharBufAlloc() override;

    CharBufAlloc & operator=(const CharBufAlloc & from) {
        CharBufBase::operator=(from);
        return *this;
    }
    CharBufAlloc & operator=(CharBufAlloc && from) {
        CharBufBase::operator=(std::move(from));
        return *this;
    }
    CharBufAlloc & operator=(char ch) {
        CharBufBase::operator=(ch);
        return *this;
    }
    CharBufAlloc & operator=(const char s[]) {
        CharBufBase::operator=(s);
        return *this;
    }
    CharBufAlloc & operator=(std::string_view str) {
        CharBufBase::operator=(str);
        return *this;
    }

    // Inherited via CharBufBase
    void * allocate(size_t bytes) final;
    void deallocate(void * ptr, size_t bytes) final;

private:
    NO_UNIQUE_ADDRESS allocator_type m_alloc;
};

//===========================================================================
template <CharAllocator A>
CharBufAlloc<A>::~CharBufAlloc() {
    // Call destruct() here so that the base class that knows what needs to be
    // destroyed can do so while that derived class providing the deallocate
    // method is still available.
    destruct();
}

//===========================================================================
template <CharAllocator A>
void * CharBufAlloc<A>::allocate(size_t bytes) {
    auto alloc = m_alloc;
    return alloc_traits::allocate(alloc, bytes);
}

//===========================================================================
template <CharAllocator A>
void CharBufAlloc<A>::deallocate(void * ptr, size_t bytes) {
    auto alloc = m_alloc;
    alloc_traits::deallocate(alloc, (char *) ptr, bytes);
}


/****************************************************************************
*
*   CharBuf
*
***/

using CharBuf = CharBufAlloc<>;

} // namespace
