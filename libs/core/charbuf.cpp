// Copyright Glen Knowles 2015 - 2024.
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
*   CharBufBase::Buffer
*
***/

//===========================================================================
CharBufBase::Buffer::Buffer ()
{}

//===========================================================================
CharBufBase::Buffer::Buffer(span<char> buf, bool fromHeap)
    : data{buf.empty() ? nullptr : buf.data()}
    , reserved{buf.empty() ? 0 : (int) buf.size() - 1}
    , mustFree{fromHeap}
{
    assert(buf.size() != 1);
}

//===========================================================================
CharBufBase::Buffer::Buffer(Buffer && from) noexcept
    : data{from.data}
    , used{from.used}
    , reserved{from.reserved}
    , mustFree{from.mustFree}
{
    from.data = nullptr;
}

//===========================================================================
CharBufBase::Buffer & CharBufBase::Buffer::operator=(
    const Buffer & from
) noexcept {
    assert(!data || !mustFree);
    assert(!from.data || !from.mustFree);
    data = from.data;
    used = from.used;
    reserved = from.reserved;
    mustFree = from.mustFree;
    return *this;
}

//===========================================================================
CharBufBase::Buffer & CharBufBase::Buffer::operator=(Buffer && from) noexcept {
    assert(!data || !mustFree);
    data = from.data;
    used = from.used;
    reserved = from.reserved;
    mustFree = from.mustFree;
    from.data = nullptr;
    return *this;
}

//===========================================================================
CharBufBase::Buffer::~Buffer() {
    assert(!data);
}


/****************************************************************************
*
*   CharBufBase
*
***/

//===========================================================================
CharBufBase::CharBufBase(CharBufBase && from) noexcept {
    swap(from);
}

//===========================================================================
CharBufBase::~CharBufBase() {
    assert(!m_size && !m_reserved);
}

//===========================================================================
CharBufBase & CharBufBase::operator=(CharBufBase && buf) noexcept {
    clear();
    swap(buf);
    return *this;
}

//===========================================================================
CharBufBase & CharBufBase::assign(size_t numCh, char ch) {
    return replace(0, m_size, numCh, ch);
}

//===========================================================================
CharBufBase & CharBufBase::assign(nullptr_t) {
    return replace(0, m_size, nullptr);
}

//===========================================================================
CharBufBase & CharBufBase::assign(const char s[]) {
    return replace(0, m_size, s);
}

//===========================================================================
CharBufBase & CharBufBase::assign(const char s[], size_t count) {
    return replace(0, m_size, s, count);
}

//===========================================================================
CharBufBase & CharBufBase::assign(string_view str, size_t pos, size_t count) {
    assert(pos <= str.size());
    return replace(
        0,
        m_size,
        str.data() + pos,
        min(count, str.size() - pos)
    );
}

//===========================================================================
CharBufBase & CharBufBase::assign(
    const CharBufBase & buf,
    size_t pos,
    size_t count
) {
    return replace(0, m_size, buf, pos, count);
}

//===========================================================================
char & CharBufBase::front() {
    assert(m_size);
    return *m_buffers.front().data;
}

//===========================================================================
const char & CharBufBase::front() const {
    assert(m_size);
    return *m_buffers.front().data;
}

//===========================================================================
char & CharBufBase::back() {
    assert(m_size);
    auto & buf = m_buffers.back();
    assert(buf.used);
    return buf.data[buf.used - 1];
}

//===========================================================================
const char & CharBufBase::back() const {
    assert(m_size);
    auto & buf = m_buffers.back();
    assert(buf.used);
    return buf.data[buf.used - 1];
}

//===========================================================================
bool CharBufBase::empty() const {
    return !m_size;
}

//===========================================================================
size_t CharBufBase::size() const {
    return m_size;
}

//===========================================================================
const char * CharBufBase::data() const {
    // Mutable access is required so the buffer can be rearranged to make the
    // requested section contiguous, but the data itself will stay unchanged.
    return const_cast<CharBufBase *>(this)->data(0, m_size);
}

//===========================================================================
const char * CharBufBase::data(size_t pos, size_t count) const {
    // Mutable access is required so the buffer can be rearranged to make the
    // requested section contiguous, but the data itself will stay unchanged.
    return const_cast<CharBufBase *>(this)->data(pos, count);
}

//===========================================================================
char * CharBufBase::data() {
    return data(0, m_size);
}

//===========================================================================
char * CharBufBase::data(size_t pos, size_t count) {
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
            auto tmp = makeBuf(bufsize);
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
        eraseBuf(myi + 1, ri + 1);
    }

    return myi->data + ic.second;
}

//===========================================================================
const char * CharBufBase::c_str() const {
    // Mutable access is required so the buffer can be rearranged to make the
    // requested section contiguous, but the data itself will stay unchanged.
    return const_cast<CharBufBase *>(this)->c_str();
}

//===========================================================================
char * CharBufBase::c_str() {
    if (empty()) {
        m_empty = 0;
        return &m_empty;
    }
    pushBack(0);
    auto ptr = data();
    popBack();
    return ptr;
}

//===========================================================================
string_view CharBufBase::view() const {
    return view(0, m_size);
}

//===========================================================================
string_view CharBufBase::view(size_t pos, size_t count) const {
    return string_view(data(pos, count), count);
}

//===========================================================================
CharBufBase::ViewIterator CharBufBase::views(size_t pos, size_t count) const {
    assert(pos <= (size_t) m_size);
    auto num = min(count, m_size - pos);
    if (!num)
        return {};
    auto ic = find(pos);
    auto vi = ViewIterator{ic.first, (size_t) ic.second, num};
    return vi;
}

//===========================================================================
void CharBufBase::clear() {
    // This is the method used by the derived class during destruction. All
    // deallocations required at destruction are done here, while the derived
    // classes deallocate is still available.
    if (m_size) {
        erase(m_buffers.data(), 0, m_size);
        deallocate(m_buffers.data(), m_reserved * sizeof (Buffer));
        m_reserved = 0;
        m_buffers = {};
    }
}

//===========================================================================
void CharBufBase::resize(size_t count) {
    if ((size_t) m_size > count) {
        erase(count);
    } else {
        insert(m_size, count - m_size, 0);
    }
}

//===========================================================================
CharBufBase & CharBufBase::insert(size_t pos, size_t numCh, char ch) {
    assert(pos <= pos + numCh && pos <= (size_t) m_size);

    // Short-circuit to avoid allocations.
    if (!numCh)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, numCh, ch);
}

//===========================================================================
CharBufBase & CharBufBase::insert(size_t pos, nullptr_t) {
    return replace(pos, 0, nullptr);
}

//===========================================================================
CharBufBase & CharBufBase::insert(size_t pos, const char s[]) {
    assert(pos <= (size_t) m_size);

    // Short-circuit to avoid allocations, find() forces buffer to exist.
    if (!*s)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, s);
}

//===========================================================================
CharBufBase & CharBufBase::insert(size_t pos, const char s[], size_t count) {
    assert(pos <= pos + count && pos <= (size_t) m_size);

    // Short-circuit to avoid allocations.
    if (!count)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, s, count);
}

//===========================================================================
CharBufBase & CharBufBase::insert(
    size_t pos,
    std::string_view str,
    size_t strPos,
    size_t strLen
) {
    assert(strPos <= str.size());
    return insert(pos, str.data() + strPos, min(strLen, str.size() - strPos));
}

//===========================================================================
CharBufBase & CharBufBase::insert(
    size_t pos,
    const CharBufBase & buf,
    size_t bufPos,
    size_t bufLen
) {
    assert(pos <= (size_t) m_size);
    assert(bufPos <= (size_t) buf.m_size);
    auto add = (int) min(bufLen, buf.m_size - bufPos);

    // Short-circuit to avoid allocations.
    if (!add)
        return *this;

    auto ic = find(pos);
    auto ic2 = buf.find(bufPos);
    return insert(ic.first, ic.second, ic2.first, ic2.second, add);
}

//===========================================================================
CharBufBase & CharBufBase::erase(size_t pos, size_t count) {
    assert(pos <= (size_t) m_size);
    auto remove = (int) min(count, m_size - pos);

    // Short-circuit to avoid allocations.
    if (!remove)
        return *this;

    auto ic = find(pos);
    return erase(ic.first, ic.second, remove);
}

//===========================================================================
CharBufBase & CharBufBase::rtrim(char ch) {
    auto myi = &m_buffers.back();
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
        eraseBuf(myi, myi + 1);
        myi -= 1;
    }
}

//===========================================================================
CharBufBase & CharBufBase::pushBack(char ch) {
    Buffer * buf = nullptr;
    if (!m_size) {
        buf = appendBuf();
    } else {
        buf = &m_buffers.back();
        if (buf->reserved == buf->used)
            buf = appendBuf();
    }

    m_size += 1;
    buf->data[buf->used++] = ch;
    return *this;
}

//===========================================================================
CharBufBase & CharBufBase::popBack() {
    assert(m_size);
    m_size -= 1;
    auto & buf = m_buffers.back();
    if (!--buf.used)
        eraseBuf(&buf, &buf + 1);
    return *this;
}

//===========================================================================
CharBufBase & CharBufBase::append(size_t numCh, char ch) {
    return insert(m_size, numCh, ch);
}

//===========================================================================
CharBufBase & CharBufBase::append(nullptr_t) {
    return insert(m_size, nullptr);
}

//===========================================================================
CharBufBase & CharBufBase::append(const char s[]) {
    return insert(m_size, s);
}

//===========================================================================
CharBufBase & CharBufBase::append(const char src[], size_t srcLen) {
    return insert(m_size, src, srcLen);
}

//===========================================================================
CharBufBase & CharBufBase::append(string_view str, size_t pos, size_t count) {
    return insert(m_size, str, pos, count);
}

//===========================================================================
CharBufBase & CharBufBase::append(
    const CharBufBase & buf,
    size_t pos,
    size_t count
) {
    return insert(m_size, buf, pos, count);
}

//===========================================================================
strong_ordering CharBufBase::compare(const char s[], size_t count) const {
    return compare(0, m_size, s, count);
}

//===========================================================================
strong_ordering CharBufBase::compare(
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
        // Compare with entire local buffer.
        if (int rc = memcmp(mydata, rdata, mycount))
            return rc < 0 ? strong_ordering::less : strong_ordering::greater;
        num -= mycount;
        if (!num)
            return mymax <=> rmax;
        // Advance to next local buffer.
        rdata += mycount;
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }

    // Compare with entire remote buffer.
    if (int rc = memcmp(mydata, rdata, num))
        return rc < 0 ? strong_ordering::less : strong_ordering::greater;
    return mymax <=> rmax;
}

//===========================================================================
strong_ordering CharBufBase::compare(string_view str) const {
    return compare(0, m_size, str.data(), str.size());
}

//===========================================================================
strong_ordering CharBufBase::compare(
    size_t pos,
    size_t count,
    string_view str
) const {
    return compare(pos, count, str.data(), str.size());
}

//===========================================================================
strong_ordering CharBufBase::compare(
    const CharBufBase & buf,
    size_t pos,
    size_t count
) const {
    return compare(0, m_size, buf, pos, count);
}

//===========================================================================
strong_ordering CharBufBase::compare(
    size_t pos,
    size_t count,
    const CharBufBase & buf,
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
            // Advance to next local buffer
            ++myi;
            mydata = myi->data;
            mycount = myi->used;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            // Advance to next remote buffer
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }
}

//===========================================================================
CharBufBase & CharBufBase::replace(
    size_t pos,
    size_t count,
    size_t numCh,
    char ch
) {
    assert(pos <= (size_t) m_size);

    auto add = numCh;
    auto remove = min(count, m_size - pos);
    auto num = min(add, remove);
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = (size_t) myi->used - ic.second;

    // Overwrite the overlap.
    for (;;) {
        auto bytes = min(num, mycount);
        memset(mydata, ch, bytes);
        if (num == bytes)
            break;
        num -= bytes;
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }

    // Erase or insert the rest.
    pos = (mydata - myi->data + num);
    if (add > remove) {
        return insert(myi, pos, add - remove, ch);
    } else {
        return erase(myi, pos, remove - add);
    }
}

//===========================================================================
CharBufBase & CharBufBase::replace(size_t pos, size_t count, nullptr_t) {
    assert(!"Attempt to add nullptr to CharBuf");
    return erase(pos, count);
}

//===========================================================================
CharBufBase & CharBufBase::replace(size_t pos, size_t count, const char s[]) {
    assert(pos <= (size_t) m_size);

    int remove = (int) min(count, m_size - pos);
    int num = remove;
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;

    // Copy the overlap, then either erase or insert the rest.
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
CharBufBase & CharBufBase::replace(
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

    // Copy the overlap.
    for (;;) {
        int bytes = min(num, mycount);
        memcpy(mydata, rdata, bytes);
        rdata += bytes;
        if (num == bytes)
            break;
        num -= bytes;
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }

    // Erase or insert the rest.
    auto mypos = (mydata - myi->data) + num;
    if (add > remove) {
        return insert(myi, mypos, rdata, add - remove);
    } else {
        return erase(myi, mypos, remove - add);
    }
}

//===========================================================================
CharBufBase & CharBufBase::replace(
    size_t pos,
    size_t count,
    const CharBufBase & src,
    size_t srcPos,
    size_t srcLen
) {
    assert(pos <= (size_t) m_size);
    assert(srcPos <= (size_t) src.m_size);

    auto add = min(srcLen, src.m_size - srcPos);
    auto remove = min(count, m_size - pos);
    auto num = min(add, remove);
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = size_t(myi->used - ic.second);
    auto ic2 = src.find(srcPos);
    auto ri = ic2.first;
    auto rdata = ri->data + ic2.second;
    auto rcount = size_t(ri->used - ic2.second);

    // Copy the overlap.
    for (;;) {
        auto bytes = min(num, min(mycount, rcount));
        memcpy(mydata, rdata, bytes);
        if (num == bytes)
            break;
        num -= bytes;
        if (mycount -= bytes) {
            mydata += bytes;
        } else {
            // Advance to next local buffer.
            ++myi;
            mydata = myi->data;
            mycount = myi->used;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            // Advance to next remote buffer.
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }

    // Erase or insert the rest.
    auto mypos = mydata - myi->data + num;
    auto rpos = rdata - ri->data + num;
    if (add > remove) {
        return insert(myi, mypos, ri, rpos, add - remove);
    } else {
        return erase(myi, mypos, remove - add);
    }
}

//===========================================================================
size_t CharBufBase::copy(char * out, size_t count, size_t pos) const {
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
void CharBufBase::swap(CharBufBase & other) {
    ::swap(m_size, other.m_size);
    ::swap(m_empty, other.m_empty);
    ::swap(m_buffers, m_buffers);
    ::swap(m_reserved, m_reserved);
}

//===========================================================================
size_t CharBufBase::defaultBlockSize() const {
    return kDefaultBlockSize;
}

//===========================================================================
// private
//===========================================================================
pair<CharBufBase::const_buffer_iterator, int> CharBufBase::find(
    size_t pos
) const {
    auto ic = const_cast<CharBufBase *>(this)->find(pos);
    return ic;
}

//===========================================================================
pair<CharBufBase::buffer_iterator, int> CharBufBase::find(size_t pos) {
    assert(pos <= (size_t) m_size);
    int off = (int)pos;
    if (off < m_size / 2) {
        auto it = &m_buffers.front();
        for (;;) {
            int used = it->used;
            if (off <= used) {
                // When on the border between two buffers we want to point
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
            appendBuf();
        int base = m_size;
        auto it = &m_buffers.back();
        for (;;) {
            base -= it->used;
            if (base <= off)
                return make_pair(it, off - base);
            --it;
        }
    }
}

//===========================================================================
// Move the data (if any) after the split point to a new block immediately
// following the block being split.
CharBufBase::buffer_iterator CharBufBase::split(
    buffer_iterator it,
    size_t pos
) {
    assert(pos <= it->used);
    auto rdata = it->data + pos;
    auto rcount = it->used - (int) pos;
    int nbufs = 0;
    while (rcount) {
        nbufs += 1;
        it = insertBuf(++it);
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
CharBufBase & CharBufBase::insert(
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
        // Fits inside the current buffer.
        if (add) {
            memmove(mydata + add, mydata, myafter);
            memset(mydata, ch, add);
            it->used += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it.
    it = split(it, pos);
    mydata = it->data + pos;
    myafter = 0;
    myavail = it->reserved - it->used;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before the
    // new block with the data that was after the insertion point.
    for (;;) {
        int bytes = min(add, myavail);
        memset(mydata, ch, bytes);
        it->used += bytes;
        add -= bytes;
        if (!add)
            return *this;
        it = insertBuf(++it);
        mydata = it->data;
        myavail = it->reserved;
    }
}

//===========================================================================
CharBufBase & CharBufBase::insert(
    buffer_iterator it,
    size_t pos,
    const char s[]
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto myafter = it->used - pos;
    auto myavail = it->reserved - it->used;

    // Search s for \0 within the number of bytes that will fit in the current
    // block.
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

    // Split the block if we're inserting into the middle of it.
    it = split(it, pos);
    mydata = it->data + pos;
    myafter = 0;
    myavail = it->reserved - it->used;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before the
    // new block with the data that was after the insertion point.
    char * mybase = mydata;
    char * myend = it->data + it->reserved;
    for (;;) {
        if (mydata == myend) {
            int bytes = int(mydata - mybase);
            it->used += bytes;
            m_size += bytes;
            it = insertBuf(++it);
            mydata = it->data;
            mybase = mydata;
            myend = mydata + it->reserved;
        }
        *mydata++ = *s++;
        if (!*s) {
            int bytes = int(mydata - mybase);
            it->used += bytes;
            m_size += bytes;
            return *this;
        }
    }
}

//===========================================================================
CharBufBase & CharBufBase::insert(
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
        // The string fits inside the current buffer.
        if (add) {
            memmove(mydata + add, mydata, myafter);
            memcpy(mydata, s, add);
            it->used += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it.
    it = split(it, pos);
    mydata = it->data + pos;
    myafter = 0;
    myavail = it->reserved - it->used;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before the
    // new block with the data that was after the insertion point.
    for (;;) {
        int bytes = min(add, myavail);
        memcpy(mydata, s, bytes);
        it->used += bytes;
        s += bytes;
        add -= bytes;
        if (!add)
            return *this;
        it = insertBuf(++it);
        mydata = it->data;
        myavail = it->reserved;
    }
}

//===========================================================================
CharBufBase & CharBufBase::insert(
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

    // Split the block if we're inserting into the middle of it.
    myi = split(myi, pos);
    mydata = myi->data + pos;
    myafter = 0;
    myavail = myi->reserved - myi->used;

    for (;;) {
        int bytes = min({add, myavail, rcount});
        memcpy(mydata, rdata, bytes);
        myi->used += bytes;
        add -= bytes;
        if (!add)
            return *this;
        if (myavail -= bytes) {
            mydata += bytes;
        } else {
            myi = insertBuf(++myi);
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
CharBufBase & CharBufBase::erase(
    buffer_iterator it,
    size_t pos,
    size_t count
) {
    assert(pos <= it->used);
    assert(m_size >= count && count >= 0);
    if (!count)
        return *this;
    auto remove = (int) count;
    auto mydata = it->data + pos;
    auto mycount = it->used - (int) pos;
    m_size -= remove;

    if (pos) {
        // Remove only from the first buffer.
        int num = mycount - remove;
        if (num > 0) {
            // Entire erase fits within fraction of first buffer.
            memmove(mydata, mydata + remove, num);
            it->used -= remove;
            return *this;
        }
        // Remove tail of first buffer.
        it->used -= mycount;
        remove -= mycount;
        if (!remove)
            return *this;

        // Erase extends past the first buffer.
        ++it;
        mydata = it->data;
        mycount = it->used;
    }

    // Remove entire buffers spanned by the erase.
    while (mycount <= remove) {
        it = eraseBuf(it, it + 1);
        remove -= mycount;
        if (!remove)
            return *this;
        mycount = it->used;
    }

    // There are bytes to remove from the beginning of one final buffer.
    mydata = it->data;
    mycount = it->used;
    int num = mycount - remove;
    memmove(mydata, mydata + remove, num);
    it->used -= remove;
    return *this;
}

//===========================================================================
auto CharBufBase::makeBuf(size_t bytes) -> Buffer {
    auto data = (char *) allocate(bytes);
    Buffer out(span(data, bytes), true);
    return out;
}

//===========================================================================
auto CharBufBase::appendBuf() -> buffer_iterator {
    return insertBuf(m_buffers.data() + m_buffers.size());
}

//===========================================================================
auto CharBufBase::insertBuf(buffer_iterator pos) -> buffer_iterator {
    auto len = m_buffers.size();
    auto first = m_buffers.data();
    auto last = first + len;
    assert(first <= pos && pos <= last);
    auto off = pos - first;
    if (len < m_reserved) {
        if (pos < last)
            memmove(pos + 1, pos, (last - pos) * sizeof *pos);
        m_buffers = { first, len + 1 };
    } else {
        m_reserved = 2 * m_reserved + 8;
        auto dfirst = (Buffer *) allocate(m_reserved * sizeof *pos);
        if (pos > first)
            memcpy(dfirst, first, off * sizeof *pos);
        if (pos < last)
            memcpy(dfirst + off + 1, pos, (last - pos) * sizeof *pos);
        pos = dfirst + off;
        m_buffers = { dfirst, len + 1 };
    }
    new(pos) Buffer(makeBuf(defaultBlockSize()));
    return pos;
}

//===========================================================================
auto CharBufBase::eraseBuf(buffer_iterator pos1, buffer_iterator pos2)
    -> buffer_iterator
{
    assert(pos1 <= pos2);
    if (pos1 == pos2)
        return pos1;
    auto len = m_buffers.size();
    auto first = m_buffers.data();
    auto last = first + len;
    assert(first <= pos1 && pos2 <= last);
    for (auto i = pos1; i < pos2; ++i) {
        if (i->data) {
            if (i->mustFree)
                deallocate(i->data, i->reserved + 1);
            i->data = nullptr;
        }
    }
    if (pos2 < last)
        memmove(pos1, pos2, (last - pos2) * sizeof *pos1);
    m_buffers = { first, len - (pos2 - pos1) };
    return pos1;
}

//===========================================================================
void * CharBufBase::allocate(size_t bytes) {
    return malloc(bytes);
}

//===========================================================================
void CharBufBase::deallocate(void * ptr, size_t bytes) {
    free(ptr);
}


/****************************************************************************
*
*   CharBufBase::ViewIterator
*
***/

//===========================================================================
CharBufBase::ViewIterator::ViewIterator(
    const_buffer_iterator buf,
    size_t pos,
    size_t count
)
    : m_current{&*buf}
    , m_view{buf->data + pos, std::min(count, buf->used - pos)}
    , m_count{count}
{
    assert(pos <= (size_t) buf->used);
    if (count + pos == buf->used)
        buf->data[buf->used] = 0;
}

//===========================================================================
bool CharBufBase::ViewIterator::operator== (const ViewIterator & right) const {
    return m_view == right.m_view;
}

//===========================================================================
CharBufBase::ViewIterator & CharBufBase::ViewIterator::operator++() {
    if (m_count -= m_view.size()) {
        ++m_current;
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
string Dim::toString(const CharBufBase & buf) {
    return toString(buf, 0, (size_t) -1);
}

//===========================================================================
string Dim::toString(const CharBufBase & buf, size_t pos) {
    return toString(buf, pos, (size_t) -1);
}

//===========================================================================
string Dim::toString(const CharBufBase & buf, size_t pos, size_t count) {
    assert(pos <= buf.size());
    count = min(count, buf.size() - pos);
    string out;
    out.resize(count);
    buf.copy(out.data(), count, pos);
    return out;
}

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const CharBufBase & buf);
}
ostream & Dim::operator<<(ostream & os, const CharBufBase & buf) {
    for (auto && v : buf.views())
        os << v;
    return os;
}
