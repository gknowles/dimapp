// charbuf.cpp - dim services
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
CharBuf::Buffer::Buffer (size_t reserve) 
    : reserved{(int) reserve}
    , data{reserve ? new char[reserve] : nullptr}
{}


/****************************************************************************
*
*   CharBuf
*
***/

//===========================================================================
CharBuf::~CharBuf() {
    for (auto && buf : m_buffers)
        delete buf.data;
}

//===========================================================================
CharBuf & CharBuf::assign(size_t count, char ch) {
    return replace(0, m_size, count, ch);
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
    assert(pos < str.size());
    return replace(
        0, m_size, str.data() + pos, min(count, str.size() - pos));
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
    return buf.data[buf.used - 1];
}

//===========================================================================
const char & CharBuf::back() const {
    assert(m_size);
    auto & buf = m_buffers.back();
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
    assert(pos <= m_size);
    if (pos == m_size)
        return nullptr;

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
            ::swap(tmp, *myi);
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
string_view CharBuf::view() const {
    return view(0, m_size);
}

//===========================================================================
string_view CharBuf::view(size_t pos, size_t count) const {
    return string_view(data(pos, count), count);
}

//===========================================================================
CharBuf::Range CharBuf::views(size_t pos, size_t count) const {
    assert(pos <= m_size);
    auto num = min(count, m_size - pos);
    auto ic = find(pos);
    auto r = Range{Iterator{ic.first, (size_t) ic.second, num}};
    return r;
}

//===========================================================================
void CharBuf::clear() {
    if (m_size)
        erase(m_buffers.begin(), 0, m_size);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, size_t numCh, char ch) {
    assert(pos <= pos + numCh && pos <= m_size);

    // short-circuit to avoid allocs
    if (!numCh)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, numCh, ch);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, const char s[]) {
    assert(pos <= m_size);

    // short-circuit to avoid allocs. find() forces allocated buffer to exist
    if (!*s)
        return *this;

    auto ic = find(pos);
    return insert(ic.first, ic.second, s);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, const char s[], size_t count) {
    assert(pos <= pos + count && pos <= m_size);

    // short-circuit to avoid allocs
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
    assert(pos <= pos + bufLen && pos <= m_size);
    assert(bufPos <= buf.m_size);

    // short-circuit to avoid allocs
    if (!bufLen)
        return *this;

    auto ic = find(pos);
    auto ic2 = buf.find(pos);
    auto add = (int) min(bufLen, buf.m_size - bufPos);
    return insert(ic.first, ic.second, ic2.first, ic2.second, add);
}

//===========================================================================
CharBuf & CharBuf::erase(size_t pos, size_t count) {
    assert(pos <= pos + count && pos + count <= m_size);

    // short-circuit to avoid allocs
    if (!count)
        return *this;

    auto ic = find(pos);
    return erase(ic.first, ic.second, (int)count);
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
void CharBuf::pushBack(char ch) {
    if (!m_size)
        m_buffers.emplace_back();

    auto buf = &m_buffers.back();
    if (buf->reserved == buf->used) {
        m_buffers.emplace_back();
        buf = &m_buffers.back();
    }

    m_size += 1;
    buf->data[buf->used++] = ch;
}

//===========================================================================
void CharBuf::popBack() {
    assert(m_size);
    m_size -= 1;
    Buffer & buf = m_buffers.back();
    if (--buf.used)
        return;

    m_buffers.pop_back();
}

//===========================================================================
CharBuf & CharBuf::append(size_t count, char ch) {
    assert(m_size <= m_size + count);
    int add = (int)count;
    if (!add)
        return *this;

    if (!m_size)
        m_buffers.emplace_back();
    m_size += add;
    for (;;) {
        auto & buf = m_buffers.back();
        auto mydata = buf.data + buf.used;
        auto mycount = buf.reserved - buf.used;
        int bytes = min(add, mycount);
        memset(mydata, ch, bytes);
        buf.used += bytes;
        add -= bytes;
        if (!add)
            return *this;
        m_buffers.emplace_back();
    }
}

//===========================================================================
CharBuf & CharBuf::append(const char s[]) {
    return insert(size(), s);
}

//===========================================================================
CharBuf & CharBuf::append(const char src[], size_t srcLen) {
    return insert(size(), src, srcLen);
}

//===========================================================================
CharBuf & CharBuf::append(string_view str, size_t pos, size_t count) {
    assert(pos <= str.size());
    return append(str.data() + pos, min(str.size(), pos + count) - pos);
}

//===========================================================================
CharBuf & CharBuf::append(const CharBuf & buf, size_t pos, size_t count) {
    return insert(size(), buf, pos, count);
}

//===========================================================================
int CharBuf::compare(const char s[], size_t count) const {
    return compare(0, m_size, s, count);
}

//===========================================================================
int CharBuf::compare(
    size_t pos, 
    size_t count, 
    const char s[], 
    size_t slen
) const {
    assert(pos <= m_size);
    int mymax = (int) min(count, m_size - pos);
    int rmax = (int) slen;
    int num = min(mymax, rmax);
    if (!num)
        return mymax - rmax;
    auto ic = find(pos);
    auto myi = ic.first;
    auto mycount = myi->used - ic.second;
    auto mydata = myi->data + ic.second;
    auto rdata = s;

    while (mycount < num) {
        // compare with entire local buffer
        int rc = memcmp(mydata, rdata, mycount);
        if (rc)
            return rc;
        num -= mycount;
        if (!num)
            return mymax - rmax;
        // advance to next local buffer
        rdata += mycount;
        ++myi;
        mydata = myi->data;
        mycount = myi->used;
    }

    // compare with entire remote buffer
    int rc = memcmp(mydata, rdata, num);
    if (rc)
        return rc;
    return mymax - rmax;
}

//===========================================================================
int CharBuf::compare(string_view str) const {
    return compare(str.data(), str.size());
}

//===========================================================================
int CharBuf::compare(size_t pos, size_t count, string_view str) const {
    return compare(pos, count, str.data(), str.size());
}

//===========================================================================
int CharBuf::compare(const CharBuf & buf, size_t pos, size_t count) const {
    return compare(0, m_size, buf, pos, count);
}

//===========================================================================
int CharBuf::compare(
    size_t pos,
    size_t count,
    const CharBuf & buf,
    size_t bufPos,
    size_t bufLen
) const {
    assert(pos <= m_size);
    assert(bufPos <= buf.size());
    int mymax = (int) min(count, m_size - pos);
    int rmax = (int) min(bufLen, buf.size() - bufPos);
    int num = min(mymax, rmax);
    if (!num)
        return mymax - rmax;
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
        int rc = memcmp(mydata, rdata, bytes);
        if (rc)
            return rc;
        num -= bytes;
        if (!num)
            return mymax - rmax;
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
    assert(pos <= m_size);

    int add = (int)numCh;
    int remove = (int) min(count, m_size - pos);
    int num = min(add, remove);
    auto ic = find(pos);
    auto myi = ic.first;
    auto mydata = myi->data + ic.second;
    auto mycount = myi->used - ic.second;

    // overwrite the overlap, then either erase or insert the rest
    for (;;) {
        int bytes = min(num, mycount);
        memset(mydata, ch, bytes);
        num -= bytes;
        if (!num) {
            int pos = int(mydata - myi->data) + bytes;
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
CharBuf & CharBuf::replace(size_t pos, size_t count, const char s[]) {
    assert(pos <= m_size);

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
            return erase(myi, int(ptr - myi->data), num - int(ptr - mydata));
        if (ptr == eptr) {
            num -= bytes;
            if (!num)
                return insert(myi, int(ptr - myi->data), s);
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
CharBuf &
CharBuf::replace(size_t pos, size_t count, const char src[], size_t srcLen) {
    assert(pos <= m_size);

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
            int mypos = int(mydata - myi->data) + bytes;
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
    assert(pos <= m_size);
    assert(srcPos <= src.m_size);

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
    assert(pos <= m_size);
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
// ITempHeap
char * CharBuf::alloc(size_t bytes, size_t align) {
    return nullptr;
}

//===========================================================================
// private
//===========================================================================
pair<vector<CharBuf::Buffer>::const_iterator, int>
CharBuf::find(size_t pos) const {
    auto ic = const_cast<CharBuf *>(this)->find(pos);
    return ic;
}

//===========================================================================
pair<vector<CharBuf::Buffer>::iterator, int> CharBuf::find(size_t pos) {
    assert(pos <= m_size);
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
vector<CharBuf::Buffer>::iterator CharBuf::split(
    vector<Buffer>::iterator it, 
    int pos
) {
    assert(pos <= it->used);
    auto rdata = it->data + pos;
    auto rcount = it->used - pos;
    int nbufs = 0;
    while (rcount) {
        nbufs += 1;
        it = m_buffers.emplace(++it);
        auto mydata = it->data;
        auto mycount = it->reserved;
        int bytes = min(mycount, rcount);
        memcpy(mydata, rdata, bytes);
        it->used = bytes;
        rcount -= bytes;
    }
    it -= nbufs;
    it->used = pos;
    return it;
}

//===========================================================================
CharBuf & CharBuf::insert(
    vector<Buffer>::iterator it,
    int pos,
    size_t numCh,
    char ch
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto mycount = it->used - pos;
    auto myavail = it->reserved - it->used;
    auto add = (int) numCh;
    m_size += add;

    if (add <= myavail) {
        // fits inside the current buffer
        if (add) {
            memmove(mydata + add, mydata, mycount);
            memset(mydata, ch, add);
            it->used += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    mydata = it->data + pos;
    mycount = 0;
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
    vector<CharBuf::Buffer>::iterator it,
    int pos,
    const char s[]
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto mycount = it->used - pos;
    auto myavail = it->reserved - it->used;

    // memchr s for \0 within the number of bytes that will fit in the
    // current block
    char * eptr = (char *)memchr(s, 0, myavail + 1);
    if (eptr) {
        // the string fits inside the current buffer
        if (int add = int(eptr - s)) {
            memmove(mydata + add, mydata, mycount);
            memcpy(mydata, s, add);
            it->used += add;
            m_size += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    mydata = it->data + pos;
    mycount = 0;
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
    vector<Buffer>::iterator it,
    int pos,
    const char s[],
    size_t slen
) {
    assert(pos <= it->used);
    auto mydata = it->data + pos;
    auto mycount = it->used - pos;
    auto myavail = it->reserved - it->used;
    auto add = (int) slen;
    m_size += add;

    if (add <= myavail) {
        // the string fits inside the current buffer
        if (add) {
            memmove(mydata + add, mydata, mycount);
            memcpy(mydata, s, add);
            it->used += add;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    mydata = it->data + pos;
    mycount = 0;
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
    vector<Buffer>::iterator myi,
    int pos,
    vector<Buffer>::const_iterator ri,
    int rpos,
    size_t rlen
) {
    assert(pos <= myi->used);
    assert(rpos <= ri->used);
    auto mycount = myi->used - pos;
    auto rcount = ri->used - rpos;
    auto num = (int) rlen;
    auto rdata = ri->data + rpos;

    if (myi->used + num <= myi->reserved) {
        auto mydata = myi->data + pos;
        memmove(mydata + num, mydata, mycount);

        for (;;) {
            int bytes = min(num, rcount);
            memcpy(mydata, rdata, bytes);
            myi->used += bytes;
            num -= bytes;
            if (!num)
                return *this;
            ++ri;
            rdata = ri->data;
            rcount = ri->used;
        }
    }

    // Split the block if we're inserting into the middle of it
    myi = split(myi, pos);

    auto mydata = myi->data + pos;
    for (;;) {
        int bytes = min(num, min(mycount, rcount));
        memcpy(mydata, rdata, bytes);
        myi->used += bytes;
        num -= bytes;
        if (!num)
            return *this;
        if (mycount -= bytes) {
            mydata += bytes;
        } else {
            ++myi;
            mydata = myi->data;
            mycount = myi->used;
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
CharBuf &
CharBuf::erase(vector<CharBuf::Buffer>::iterator it, int pos, int remove) {
    assert(pos <= it->used);
    assert(m_size >= remove && remove >= 0);
    if (!remove)
        return *this;
    auto mydata = it->data + pos;
    auto mycount = it->used - pos;
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
*   Free functions
*
***/

//===========================================================================
bool Dim::operator==(const CharBuf & left, string_view right) {
    return left.compare(right) == 0;
}

//===========================================================================
bool Dim::operator==(string_view left, const CharBuf & right) {
    return right.compare(left) == 0;
}

//===========================================================================
bool Dim::operator==(const CharBuf & left, const CharBuf & right) {
    return left.compare(right) == 0;
}

//===========================================================================
string Dim::to_string(const CharBuf & buf, size_t pos, size_t count) {
    assert(pos <= buf.size());
    count = min(count, buf.size() - pos);
    string out;
    out.resize(count);
    buf.copy(out.data(), count, pos);
    return out;
}
