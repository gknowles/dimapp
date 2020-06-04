// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// charbuf.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const unsigned kDefaultBlockSize = 4096;


/****************************************************************************
*
*   CharBuf::Buffer
*
***/

//===========================================================================
CharBuf::Buffer::Buffer ()
    : Buffer{kDefaultBlockSize}
{}

//===========================================================================
CharBuf::Buffer::Buffer(size_t reserve)
    : data{reserve ? new char[reserve + 1] : nullptr}
    , reserved{(int) reserve}
    , heapUsed{true}
{}

//===========================================================================
CharBuf::Buffer::Buffer(Buffer && from) noexcept
    : data{from.data}
    , used{from.used}
    , reserved{from.reserved}
    , heapUsed{from.heapUsed}
{
    from.data = nullptr;
}

//===========================================================================
CharBuf::Buffer & CharBuf::Buffer::operator=(Buffer && from) noexcept {
    if (data && heapUsed)
        delete[] data;
    data = from.data;
    from.data = nullptr;
    used = from.used;
    reserved = from.reserved;
    heapUsed = from.heapUsed;
    return *this;
}

//===========================================================================
CharBuf::Buffer::~Buffer() {
    if (data) {
        if (heapUsed)
            delete[] data;
        data = nullptr;
    }
}


/****************************************************************************
*
*   CharBuf
*
***/

//===========================================================================
CharBuf::~CharBuf()
{}

//===========================================================================
CharBuf & CharBuf::assign(size_t numCh, char ch) {
    return replace(0, m_size, numCh, ch);
}

//===========================================================================
CharBuf & CharBuf::assign(nullptr_t) {
    return replace(0, m_size, nullptr);
}

//===========================================================================
CharBuf & CharBuf::assign(const char s[]) {
    return replace(0, m_size, s);
}

//===========================================================================
CharBuf & CharBuf::assign(const char s[], size_t count) {
    return replace(0, m_size, s, count);
}

//===========================================================================
CharBuf & CharBuf::assign(string_view str, size_t pos, size_t count) {
    assert(pos <= str.size());
    return replace(
        0,
        m_size,
        str.data() + pos,
        min(count, str.size() - pos)
    );
}

//===========================================================================
CharBuf & CharBuf::assign(const CharBuf & buf, size_t pos, size_t count) {
    return replace(0, m_size, buf, pos, count);
}

//===========================================================================
char & CharBuf::front() {
    assert(m_size);
    return *m_buffers.front().data;
}

//===========================================================================
const char & CharBuf::front() const {
    assert(m_size);
    return *m_buffers.front().data;
}

//===========================================================================
char & CharBuf::back() {
    assert(m_size);
    auto & buf = m_buffers.back();
    assert(buf.used);
    return buf.data[buf.used - 1];
}

//===========================================================================
const char & CharBuf::back() const {
    assert(m_size);
    auto & buf = m_buffers.back();
    assert(buf.used);
    return buf.data[buf.used - 1];
}

//===========================================================================
bool CharBuf::empty() const {
    return !m_size;
}

//===========================================================================
size_t CharBuf::size() const {
    return m_size;
}

//===========================================================================
const char * CharBuf::data() const {
    // we need mutable access to rearrange the buffer so that the requested
    // section is contiguous, but the data itself will stay unchanged
    return const_cast<CharBuf *>(this)->data(0, m_size);
}

//===========================================================================
const char * CharBuf::data(size_t pos, size_t count) const {
    // we need mutable access to rearrange the buffer so that the requested
    // section is contiguous, but the data itself will stay unchanged
    return const_cast<CharBuf *>(this)->data(pos, count);
}

//===========================================================================
char * CharBuf::data() {
    return data(0, m_size);
}

//===========================================================================
char * CharBuf::data(size_t pos, size_t count) {
    assert(pos <= (size_t) m_size);
    if (pos == (size_t) m_size)
        return nullptr;

    count = min<size_t>(count, m_size);
    auto need = (int) min(count, m_size - pos);
    auto ic = find(pos);
    auto myi = ic.first;
    auto num = ic.second + need;

    if (num > myi->used) {
        if (num > myi->reserved) {
            size_t bufsize = count + kDefaultBlockSize - 1;
            bufsize -= bufsize % kDefaultBlockSize;
            Buffer tmp(bufsize);
            memcpy(tmp.data, myi->data, myi->used);
            tmp.used = myi->used;
            *myi = move(tmp);
        }
        num -= myi->used;
        auto mydata = myi->data + myi->used;

        auto ri = myi;
        for (;;) {
            ++ri;
            memcpy(mydata, ri->data, ri->used);
            myi->used += ri->used;
            num -= ri->used;
            if (!num)
                break;
            mydata += ri->used;
        }
        m_buffers.erase(myi + 1, ri + 1);
    }

    return myi->data + ic.second;
}

//===========================================================================
const char * CharBuf::c_str() const {
    // we need mutable access to rearrange the buffer so that the requested
    // section is contiguous, but the data itself will stay unchanged
    return const_cast<CharBuf *>(this)->c_str();
}

//===========================================================================
char * CharBuf::c_str() {
    if (empty()) {
        m_empty = '\0';
        return &m_empty;
    }
    pushBack(0);
    auto ptr = data();
    popBack();
    return ptr;
}

//===========================================================================
string_view CharBuf::view() const {
    return view(0, m_size);
}

//===========================================================================
string_view CharBuf::view(size_t pos, size_t count) const {
    return string_view(data(pos, count), count);
}

//===========================================================================
CharBuf::ViewIterator CharBuf::views(size_t pos, size_t count) const {
    assert(pos <= (size_t) m_size);
    auto num = min(count, m_size - pos);
    if (!num)
        return {};
    auto ic = find(pos);
    auto vi = ViewIterator{ic.first, (size_t) ic.second, num};
    return vi;
}

//===========================================================================
void CharBuf::clear() {
    if (m_size)
        erase(m_buffers.begin(), 0, m_size);
}

//===========================================================================
void CharBuf::resize(size_t count) {
    if ((size_t) m_size > count) {
        erase(count);
    } else {
        insert(m_size, count - m_size, 0);
    }
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, size_t numCh, char ch) {
    assert(pos <= pos + numCh && pos <= (size_t) m_size);

    // short-circuit to avoid allocations
    if (!numCh)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, numCh, ch);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, nullptr_t) {
    return replace(pos, 0, nullptr);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, const char s[]) {
    assert(pos <= (size_t) m_size);

    // short-circuit to avoid allocations. find() forces buffer to exist
    if (!*s)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, s);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, const char s[], size_t count) {
    assert(pos <= pos + count && pos <= (size_t) m_size);

    // short-circuit to avoid allocations
    if (!count)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, s, count);
}

//===========================================================================
CharBuf & CharBuf::insert(
    size_t pos,
    const CharBuf & buf,
    size_t bufPos,
    size_t bufLen
) {
    assert(pos <= (size_t) m_size);
    assert(bufPos <= (size_t) buf.m_size);
    auto add = (int) min(bufLen, buf.m_size - bufPos);

    // short-circuit to avoid allocations
    if (!add)
        return *this;

    auto ic = find(pos);
    auto ic2 = buf.find(bufPos);
    return insert(ic.first, ic.second, ic2.first, ic2.second, add);
}

//===========================================================================
CharBuf & CharBuf::erase(size_t pos, size_t count) {
    assert(pos <= (size_t) m_size);
    auto remove = (int) min(count, m_size - pos);

    // short-circuit to avoid allocations
    if (!remove)
        return *this;

    auto ic = find(pos);
    return erase(ic.first, ic.second, remove);
}

//===========================================================================
CharBuf & CharBuf::rtrim(char ch) {
    auto myi = m_buffers.end();
    --myi;
    for (;;) {
        if (!m_size)
            return *this;
        auto base = myi->data;
        auto eptr = base + myi->used - 1;
        auto ptr = eptr;
        for (;;) {
            if (*ptr != ch) {
                if (auto num = int(eptr - ptr)) {
                    m_size -= num;
                    myi->used -= num;
                }
                return *this;
            }
            if (ptr == base)
                break;
            ptr -= 1;
        }
        m_size -= myi->used;
        m_buffers.erase(myi--);
    }
}

//===========================================================================
CharBuf & CharBuf::pushBack(char ch) {
    Buffer * buf = nullptr;
    if (!m_size) {
        buf = &m_buffers.emplace_back();
    } else {
        buf = &m_buffers.back();
        if (buf->reserved == buf->used)
            buf = &m_buffers.emplace_back();
    }

    m_size += 1;
    buf->data[buf->used++] = ch;
    return *this;
}

//===========================================================================
CharBuf & CharBuf::popBack() {
    assert(m_size);
    m_size -= 1;
    auto & buf = m_buffers.back();
    if (!--buf.used)
        m_buffers.pop_back();
    return *this;
}

//===========================================================================
CharBuf & CharBuf::append(size_t numCh, char ch) {
    return insert(m_size, numCh, ch);
}

//===========================================================================
CharBuf & CharBuf::append(nullptr_t) {
    return insert(m_size, nullptr);
}

//===========================================================================
CharBuf & CharBuf::append(const char s[]) {
    return insert(m_size, s);
}

//===========================================================================
CharBuf & CharBuf::append(const char src[], size_t srcLen) {
    return insert(m_size, src, srcLen);
}

//===========================================================================
CharBuf & CharBuf::append(string_view str, size_t pos, size_t count) {
    assert(pos <= str.size());
    return append(str.data() + pos, min(str.size(), pos + count) - pos);
}

//===========================================================================
CharBuf & CharBuf::append(const CharBuf & buf, size_t pos, size_t count) {
    return insert(m_size, buf, pos, count);
}

//===========================================================================
strong_ordering CharBuf::compare(const char s[], size_t count) const {
    return compare(0, m_size, s, count);
}

//===========================================================================
strong_ordering CharBuf::compare(
    size_t pos,
    size_t count,
    const char s[],
    size_t slen
) const {
    assert(pos <= (size_t) m_size);
    int mymax = (int) min(count, m_size - pos);
    int rmax = (int) slen;
    int num = min(mymax, rmax);
    if (!num)
        return mymax <=> rmax;
    auto ic = find(pos);
    auto myi = ic.first;
    auto mycount = myi->used - ic.second;
    auto mydata = myi->data + ic.second;
    auto rdata = s;

    while (mycount < num) {
        // compare with entire local buffer
        if (int rc = memcmp(mydata, rdata, mycount))
            return rc < 0 ? strong_ordering::less : strong_ordering::greater;
        num -= mycount;
        if (!num)
            return mymax <=> rmax;
        // advance to next local buffer
        rdata += mycount;
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }

    // compare with entire remote buffer
    if (int rc = memcmp(mydata, rdata, num))
        return rc < 0 ? strong_ordering::less : strong_ordering::greater;
    return mymax <=> rmax;
}

//===========================================================================
strong_ordering CharBuf::compare(string_view str) const {
    return compare(0, m_size, str.data(), str.size());
}

//===========================================================================
strong_ordering CharBuf::compare(size_t pos, size_t count, string_view str) const {
    return compare(pos, count, str.data(), str.size());
}

//===========================================================================
strong_ordering CharBuf::compare(const CharBuf & buf, size_t pos, size_t count) const {
    return compare(0, m_size, buf, pos, count);
}

//===========================================================================
strong_ordering CharBuf::compare(
    size_t pos,
    size_t count,
    const CharBuf & buf,
    size_t bufPos,
    size_t bufLen
) const {
    assert(pos <= (size_t) m_size);
    assert(bufPos <= (size_t) buf.m_size);
    int mymax = (int) min(count, m_size - pos);
    int rmax = (int) min(bufLen, buf.size() - bufPos);
    int num = min(mymax, rmax);
    if (!num)
        return mymax <=> rmax;
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;
    ic = buf.find(bufPos);
    auto ri = ic.first;
    auto rdata = ri->data + ic.second;
    auto rcount = ri->used - ic.second;

    for (;;) {
        int bytes = min(num, min(mycount, rcount));
        if (int rc = memcmp(mydata, rdata, bytes))
            return rc < 0 ? strong_ordering::less : strong_ordering::greater;
        num -= bytes;
        if (!num)
            return mymax <=> rmax;
        if (mycount -= bytes) {
            mydata += bytes;
        } else {
            // advance to next local buffer
            ++myi;
            mydata = myi->data;
            mycount = myi->used;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            // advance to next remote buffer
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }
}

//===========================================================================
CharBuf & CharBuf::replace(size_t pos, size_t count, size_t numCh, char ch) {
    assert(pos <= (size_t) m_size);

    auto add = numCh;
    auto remove = min(count, m_size - pos);
    auto num = min(add, remove);
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = (size_t) myi->used - ic.second;

    // overwrite the overlap, then either erase or insert the rest
    for (;;) {
        auto bytes = min(num, mycount);
        memset(mydata, ch, bytes);
        num -= bytes;
        if (!num) {
            auto pos = (int) (mydata - myi->data + bytes);
            return add > remove
                ? insert(myi, pos, add - remove, ch)
                : erase(myi, pos, remove - add);
        }
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }
}

//===========================================================================
CharBuf & CharBuf::replace(size_t pos, size_t count, nullptr_t) {
    assert(!"Attempt to add nullptr to CharBuf");
    return erase(pos, count);
}

//===========================================================================
CharBuf & CharBuf::replace(size_t pos, size_t count, const char s[]) {
    assert(pos <= (size_t) m_size);

    int remove = (int) min(count, m_size - pos);
    int num = remove;
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;

    // copy the overlap, then either erase or insert the rest
    char * ptr = mydata;
    int bytes = min(num, mycount);
    char * eptr = ptr + bytes;
    for (;;) {
        if (!*s)
            return erase(myi, ptr - myi->data, num - (ptr - mydata));
        if (ptr == eptr) {
            num -= bytes;
            if (!num)
                return insert(myi, ptr - myi->data, s);
            ++myi;
            mydata = myi->data;
            mycount = myi->used;
            ptr = mydata;
            bytes = min(num, mycount);
            eptr = ptr + bytes;
        }
        *ptr++ = *s++;
    }
}

//===========================================================================
CharBuf & CharBuf::replace(
    size_t pos,
    size_t count,
    const char src[],
    size_t srcLen
) {
    assert(pos <= (size_t) m_size);

    int add = (int)srcLen;
    int remove = (int) min(count, m_size - pos);
    int num = min(add, remove);
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;
    auto rdata = src;

    // copy the overlap, then either erase or insert the rest
    for (;;) {
        int bytes = min(num, mycount);
        memcpy(mydata, rdata, bytes);
        rdata += bytes;
        num -= bytes;
        if (!num) {
            auto mypos = (mydata - myi->data) + bytes;
            return add > remove
                ? insert(myi, mypos, rdata, add - remove)
                : erase(myi, mypos, remove - add);
        }
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }

}

//===========================================================================
CharBuf & CharBuf::replace(
    size_t pos,
    size_t count,
    const CharBuf & src,
    size_t srcPos,
    size_t srcLen
) {
    assert(pos <= (size_t) m_size);
    assert(srcPos <= (size_t) src.m_size);

    int add = (int) min(srcLen, src.m_size - srcPos);
    int remove = (int) min(count, m_size - pos);
    int num = min(add, remove);
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;
    auto ic2 = src.find(srcPos);
    auto ri = ic2.first;
    auto rdata = ri->data + ic2.second;
    auto rcount = ri->used - ic2.second;

    // copy the overlap, then either erase or insert the rest
    for (;;) {
        int bytes = min(num, min(mycount, rcount));
        memcpy(mydata, rdata, bytes);
        num -= bytes;
        if (!num) {
            int mypos = int(mydata - myi->data) + bytes;
            int rpos = int(rdata - ri->data) + bytes;
            return add > remove
                ? insert(myi, mypos, ri, rpos, add - remove)
                : erase(myi, mypos, remove - add);
        }
        if (mycount -= bytes) {
            mydata += bytes;
        } else {
            // advance to next local buffer
            ++myi;
            mydata = myi->data;
            mycount = myi->used;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            // advance to next remote buffer
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }
}

//===========================================================================
size_t CharBuf::copy(char * out, size_t count, size_t pos) const {
    assert(pos <= (size_t) m_size);
    count = min(count, m_size - pos);
    auto num = (int) count;
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;
    for (;;) {
        auto bytes = min(num, mycount);
        memcpy(out, mydata, bytes);
        num -= bytes;
        if (!num)
            break;
        out += bytes;
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }
    return count;
}

//===========================================================================
void CharBuf::swap(CharBuf & other) {
    ::swap(m_buffers, other.m_buffers);
    ::swap(m_lastUsed, other.m_lastUsed);
    ::swap(m_size, other.m_size);
}

//===========================================================================
size_t CharBuf::defaultBlockSize() const {
    return kDefaultBlockSize;
}

//===========================================================================
// ITempHeap
char * CharBuf::alloc(size_t bytes, size_t align) {
    return nullptr;
}

//===========================================================================
// private
//===========================================================================
pair<CharBuf::const_buffer_iterator, int> CharBuf::find(size_t pos) const {
    auto ic = const_cast<CharBuf *>(this)->find(pos);
    return ic;
}

//===========================================================================
pair<CharBuf::buffer_iterator, int> CharBuf::find(size_t pos) {
    assert(pos <= (size_t) m_size);
    int off = (int)pos;
    if (off < m_size / 2) {
        auto it = m_buffers.begin();
        for (;;) {
            int used = it->used;
            if (off <= used) {
                // when on the border between two buffers we want to point
                // at the start of the later one rather than one past the end
                // of the earlier.
                if (off < used || used < it->reserved)
                    return make_pair(it, off);
            }
            off -= used;
            ++it;
        }
    } else {
        if (!m_size)
            m_buffers.emplace_back();
        int base = m_size;
        auto it = m_buffers.end();
        for (;;) {
            --it;
            base -= it->used;
            if (base <= off)
                return make_pair(it, off - base);
        }
    }
}

//===========================================================================
// Move the data (if any) after the split point to a new block immediately
// following the block being split.
CharBuf::buffer_iterator CharBuf::split(buffer_iterator it, size_t pos) {
    assert(pos <= it->used);
    auto rdata = it->data + pos;
    auto rcount = it->used - (int) pos;
    int nbufs = 0;
    while (rcount) {
        nbufs += 1;
        it = m_buffers.emplace(++it);
        auto mydata = it->data;
        auto mycount = it->reserved;
        int bytes = min(mycount, rcount);
        memcpy(mydata, rdata, bytes);
        it->used = bytes;
        rdata += bytes;
        rcount -= bytes;
    }
    it -= nbufs;
    it->used = (int) pos;
    return it;
}

//===========================================================================
CharBuf & CharBuf::insert(
    buffer_iterator it,
    size_t pos,
    size_t numCh,
    char ch
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto myafter = it->used - pos;
    auto myavail = it->reserved - it->used;
    auto add = (int) numCh;
    m_size += add;

    if (add <= myavail) {
        // fits inside the current buffer
        if (add) {
            memmove(mydata + add, mydata, myafter);
            memset(mydata, ch, add);
            it->used += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    mydata = it->data + pos;
    myafter = 0;
    myavail = it->reserved - it->used;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before
    // the new block with the data that was after the insertion point.
    for (;;) {
        int bytes = min(add, myavail);
        memset(mydata, ch, bytes);
        it->used += bytes;
        add -= bytes;
        if (!add)
            return *this;
        it = m_buffers.emplace(++it);
        mydata = it->data;
        myavail = it->reserved;
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    buffer_iterator it,
    size_t pos,
    const char s[]
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto myafter = it->used - pos;
    auto myavail = it->reserved - it->used;

    // memchr s for \0 within the number of bytes that will fit in the
    // current block
    char * eptr = (char *)memchr(s, 0, myavail + 1);
    if (eptr) {
        // the string fits inside the current buffer
        if (int add = int(eptr - s)) {
            memmove(mydata + add, mydata, myafter);
            memcpy(mydata, s, add);
            it->used += add;
            m_size += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    mydata = it->data + pos;
    myafter = 0;
    myavail = it->reserved - it->used;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before
    // the new block with the data that was after the insertion point.
    char * mybase = mydata;
    char * myend = it->data + it->reserved;
    for (;;) {
        *mydata++ = *s++;
        if (!*s) {
            int bytes = int(mydata - mybase);
            it->used += bytes;
            m_size += bytes;
            return *this;
        }
        if (mydata == myend) {
            int bytes = int(mydata - mybase);
            it->used += bytes;
            m_size += bytes;
            it = m_buffers.emplace(++it);
            mydata = it->data;
            mybase = mydata;
            myend = mydata + it->reserved;
        }
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    buffer_iterator it,
    size_t pos,
    const char s[],
    size_t slen
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto myafter = it->used - pos;
    auto myavail = it->reserved - it->used;
    auto add = (int) slen;
    m_size += add;

    if (add <= myavail) {
        // the string fits inside the current buffer
        if (add) {
            memmove(mydata + add, mydata, myafter);
            memcpy(mydata, s, add);
            it->used += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    mydata = it->data + pos;
    myafter = 0;
    myavail = it->reserved - it->used;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before
    // the new block with the data that was after the insertion point.
    for (;;) {
        int bytes = min(add, myavail);
        memcpy(mydata, s, bytes);
        it->used += bytes;
        s += bytes;
        add -= bytes;
        if (!add)
            return *this;
        it = m_buffers.emplace(++it);
        mydata = it->data;
        myavail = it->reserved;
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    buffer_iterator myi,
    size_t pos,
    const_buffer_iterator ri,
    size_t rpos,
    size_t rlen
) {
    assert(pos <= myi->used);
    assert(rpos <= ri->used);
    auto mydata = myi->data + pos;
    auto myafter = myi->used - pos;
    auto myavail = myi->reserved - myi->used;
    auto rdata = ri->data + rpos;
    auto rcount = ri->used - (int) rpos;
    auto add = (int) rlen;
    m_size += add;

    if (add <= myavail) {
        memmove(mydata + add, mydata, myafter);

        for (;;) {
            int bytes = min(add, rcount);
            memcpy(mydata, rdata, bytes);
            myi->used += bytes;
            add -= bytes;
            if (!add)
                return *this;
            mydata += bytes;
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }

    // Split the block if we're inserting into the middle of it
    myi = split(myi, pos);
    mydata = myi->data + pos;
    myafter = 0;
    myavail = myi->reserved - myi->used;

    for (;;) {
        int bytes = min(add, min(myavail, rcount));
        memcpy(mydata, rdata, bytes);
        myi->used += bytes;
        add -= bytes;
        if (!add)
            return *this;
        if (myavail -= bytes) {
            mydata += bytes;
        } else {
            myi = m_buffers.emplace(++myi);
            mydata = myi->data;
            myavail = myi->reserved;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }
}

//===========================================================================
CharBuf & CharBuf::erase(buffer_iterator it, size_t pos, size_t count) {
    assert(pos <= it->used);
    assert(m_size >= count && count >= 0);
    if (!count)
        return *this;
    auto remove = (int) count;
    auto mydata = it->data + pos;
    auto mycount = it->used - (int) pos;
    m_size -= remove;

    // remove only part of first buffer?
    if (pos) {
        int num = mycount - remove;
        if (num > 0) {
            // entire erase fits within fraction of first buffer
            memmove(mydata, mydata + remove, num);
            it->used -= remove;
            return *this;
        }
        // remove tail of first buffer
        it->used -= mycount;
        remove -= mycount;
        if (!remove)
            return *this;

        // erase extends past the first buffer
        ++it;
        mydata = it->data;
        mycount = it->used;
    }

    // remove entire buffers spanned by the erase
    while (mycount <= remove) {
        it = m_buffers.erase(it);
        remove -= mycount;
        if (!remove)
            return *this;
        mycount = it->used;
    }

    // there are bytes to remove from the beginning of one final buffer
    mydata = it->data;
    mycount = it->used;
    int num = mycount - remove;
    memmove(mydata, mydata + remove, num);
    it->used -= remove;
    return *this;
}


/****************************************************************************
*
*   CharBuf::ViewIterator
*
***/

//===========================================================================
CharBuf::ViewIterator::ViewIterator(
    const_buffer_iterator buf,
    size_t pos,
    size_t count
)
    : m_current{buf}
    , m_view{buf->data + pos, std::min(count, buf->used - pos)}
    , m_count{count}
{
    assert(pos <= (size_t) buf->used);
    if (count + pos == buf->used)
        buf->data[buf->used] = 0;
}

//===========================================================================
bool CharBuf::ViewIterator::operator== (const ViewIterator & right) const {
    return m_current == right.m_current && m_view == right.m_view;
}

//===========================================================================
CharBuf::ViewIterator & CharBuf::ViewIterator::operator++() {
    if (m_count -= m_view.size()) {
        ++m_current;
        assert(m_current != const_buffer_iterator{});
        m_view = {
            m_current->data,
            std::min(m_count, (size_t) m_current->used)
        };
        if (m_count >= m_current->used)
            m_current->data[m_current->used] = 0;
    } else {
        m_current = {};
        m_view = {};
    }
    return *this;
}


/****************************************************************************
*
*   Free functions
*
***/

//===========================================================================
string Dim::toString(const CharBuf & buf, size_t pos, size_t count) {
    assert(pos <= buf.size());
    count = min(count, buf.size() - pos);
    string out;
    out.resize(count);
    buf.copy(out.data(), count, pos);
    return out;
}

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const CharBuf & buf);
}
ostream & Dim::operator<<(ostream & os, const CharBuf & buf) {
    for (auto && v : buf.views())
        os << v;
    return os;
}
