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

struct CharBuf::Buffer {
    int m_used{0};
    int m_reserved{kDefaultBlockSize};
    bool m_heapUsed{false};
    char m_data[1];

    char * base() { return m_data; }
    char * unused() { return base() + m_used; }
    char * end() { return base() + m_reserved; }
};


/****************************************************************************
*
*   CharBuf
*
***/

//===========================================================================
CharBuf::CharBuf() {}

//===========================================================================
CharBuf::~CharBuf() {
    for (auto && ptr : m_buffers)
        delete ptr;
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
        0, m_size, str.data() + pos, min(str.size(), pos + count) - pos);
}

//===========================================================================
CharBuf & CharBuf::assign(const CharBuf & buf, size_t pos, size_t count) {
    return replace(0, m_size, buf, pos, count);
}

//===========================================================================
char & CharBuf::front() {
    assert(m_size);
    auto & buf = *m_buffers.front();
    return *buf.m_data;
}

//===========================================================================
const char & CharBuf::front() const {
    assert(m_size);
    auto & buf = *m_buffers.front();
    return *buf.m_data;
}

//===========================================================================
char & CharBuf::back() {
    assert(m_size);
    auto & buf = *m_buffers.back();
    return buf.m_data[buf.m_used - 1];
}

//===========================================================================
const char & CharBuf::back() const {
    assert(m_size);
    auto & buf = *m_buffers.back();
    return buf.m_data[buf.m_used - 1];
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

    count = min(count, m_size - pos);
    auto ic = find(pos);
    size_t need = ic.second + count;
    Buffer * pbuf = *ic.first;

    if (need > pbuf->m_used) {
        if (need > pbuf->m_reserved) {
            size_t bufsize = count + kDefaultBlockSize - 1;
            bufsize -= bufsize % kDefaultBlockSize;
            Buffer * out = allocBuffer(bufsize);
            memcpy(out->base(), pbuf->base(), pbuf->m_used);
            out->m_used = pbuf->m_used;
            delete pbuf;
            *ic.first = out;
            pbuf = out;
        }

        auto et = ic.first;
        do {
            ++et;
            Buffer * ebuf = *et;
            memcpy(pbuf->unused(), ebuf->base(), ebuf->m_used);
            pbuf->m_used += ebuf->m_used;
            delete ebuf;
        } while (pbuf->m_used < need);

        m_buffers.erase(ic.first + 1, et + 1);
    }

    return pbuf->base() + ic.second;
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
    auto it = m_buffers.end();
    --it;
    for (;;) {
        Buffer * buf = *it;
        if (!m_size)
            return *this;
        const char * base = buf->base();
        const char * ptr = buf->unused() - 1;
        for (;;) {
            if (*ptr != ch) {
                int num = int(buf->unused() - ptr - 1);
                if (num) {
                    m_size -= num;
                    if ((buf->m_used -= num) == 0) {
                        delete buf;
                        m_buffers.erase(it);
                    }
                }
                return *this;
            }
            if (ptr == base)
                break;
            ptr -= 1;
        }
        m_size -= buf->m_used;
        auto tmp = it;
        --it;
        delete buf;
        m_buffers.erase(tmp);
    }
}

//===========================================================================
void CharBuf::pushBack(char ch) {
    Buffer * buf;
    if (!m_size) {
        buf = allocBuffer();
        m_buffers.emplace_back(buf);
    } else {
        buf = m_buffers.back();
        if (buf->m_reserved == buf->m_used) {
            buf = allocBuffer();
            m_buffers.emplace_back(buf);
        }
    }
    m_size += 1;
    buf->m_data[buf->m_used++] = ch;
}

//===========================================================================
void CharBuf::popBack() {
    assert(m_size);
    m_size -= 1;
    Buffer * buf = m_buffers.back();
    if (--buf->m_used)
        return;

    delete buf;
    m_buffers.pop_back();
}

//===========================================================================
CharBuf & CharBuf::append(size_t count, char ch) {
    assert(m_size <= m_size + count);
    int add = (int)count;
    if (!add)
        return *this;

    if (!m_size)
        m_buffers.emplace_back(allocBuffer());
    m_size += add;
    for (;;) {
        Buffer * buf = m_buffers.back();
        int added = min(add, buf->m_reserved - buf->m_used);
        if (added) {
            char * ptr = buf->unused();
            memset(ptr, ch, added);
            buf->m_used += added;
            add -= added;
            if (!add)
                return *this;
        }
        m_buffers.emplace_back(allocBuffer());
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
    assert(pos < str.size());
    return append(str.data() + pos, min(str.size(), pos + count) - pos);
}

//===========================================================================
CharBuf & CharBuf::append(const CharBuf & buf, size_t pos, size_t count) {
    assert(pos < buf.size());
    if (!count)
        return *this;
    auto ic = find(pos);
    auto eb = m_buffers.end();
    int add = (int)count;
    Buffer * ptr = *ic.first;
    int copied = min(ptr->m_used - ic.second, add);
    append(ptr->m_data + ic.second, copied);

    for (;;) {
        add -= copied;
        if (!add || ++ic.first == eb)
            return *this;
        ptr = *ic.first;
        copied = min(ptr->m_used, add);
        append(ptr->m_data, copied);
    }
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
    auto mycount = (*myi)->m_used - ic.second;
    const char * mydata = (*myi)->m_data + ic.second;
    const char * rdata = s;

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
        mydata = (*++myi)->m_data;
        mycount = (*myi)->m_used;
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
    int mycount = (*myi)->m_used - ic.second;
    const char * mydata = (*myi)->m_data + ic.second;
    ic = buf.find(bufPos);
    auto ri = ic.first;
    int rcount = (*ri)->m_used - ic.second;
    const char * rdata = (*ri)->m_data + ic.second;

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
            mydata = (*++myi)->m_data;
            mycount = (*myi)->m_used;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            // advance to next remote buffer
            rdata = (*++ri)->m_data;
            rcount = (*ri)->m_used;
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
    char * mydata = (*myi)->m_data + ic.second;
    int mycount = (*myi)->m_used - ic.second;

    // overwrite the overlap, then either erase or insert the rest
    for (;;) {
        int bytes = min(num, mycount);
        memset(mydata, ch, bytes);
        num -= bytes;
        if (!num) {
            int pos = int(mydata - (*myi)->m_data) + bytes;
            return add > remove
                ? insert(myi, pos, add - remove, ch)
                : erase(myi, pos, remove - add);
        }
        mydata = (*++myi)->m_data;
        mycount = (*myi)->m_used;
    }
}

//===========================================================================
CharBuf & CharBuf::replace(size_t pos, size_t count, const char s[]) {
    assert(pos <= m_size);

    int remove = (int) min(count, m_size - pos);
    int num = remove;
    auto ic = find(pos);
    auto myi = ic.first;
    char * mydata = (*myi)->m_data + ic.second;
    int mycount = (*myi)->m_used - ic.second;

    // copy the overlap, then either erase or insert the rest
    char * ptr = mydata;
    int bytes = min(num, mycount);
    char * eptr = ptr + bytes;
    for (;;) {
        if (!*s) {
            return erase(
                ic.first, 
                int(ptr - (*myi)->m_data), 
                num - int(ptr - mydata)
            );
        }
        if (ptr == eptr) {
            num -= bytes;
            if (!num)
                return insert(ic.first, int(ptr - (*myi)->m_data), s);
            mydata = (*++myi)->m_data;
            mycount = (*myi)->m_used;
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
    char * mydata = (*myi)->m_data + ic.second;
    int mycount = (*myi)->m_used - ic.second;
    const char * rdata = src;

    // copy the overlap, then either erase or insert the rest
    for (;;) {
        int bytes = min(num, mycount);
        memcpy(mydata, rdata, bytes);
        rdata += bytes;
        num -= bytes;
        if (!num) {
            int mypos = int(mydata - (*myi)->m_data) + bytes;
            return add > remove
                ? insert(myi, mypos, rdata, add - remove)
                : erase(myi, mypos, remove - add);
        }
        mydata = (*++myi)->m_data;
        mycount = (*myi)->m_used;
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
    auto mydata = (*myi)->m_data + ic.second;
    auto mycount = (*myi)->m_used - ic.second;
    auto ic2 = src.find(srcPos);
    auto ri = ic2.first;
    auto rdata = (*ri)->m_data + ic2.second;
    auto rcount = (*ri)->m_used - ic2.second;

    // copy the overlap, then either erase or insert the rest
    for (;;) {
        int bytes = min(num, min(mycount, rcount));
        memcpy(mydata, rdata, bytes);
        num -= bytes;
        if (!num) {
            int mypos = int(mydata - (*myi)->m_data) + bytes;
            int rpos = int(rdata - (*ri)->m_data) + bytes;
            return add > remove
                ? insert(myi, mypos, ri, rpos, add - remove)
                : erase(myi, mypos, remove - add);
        }
        if (mycount -= bytes) {
            mydata += bytes;
        } else {
            // advance to next local buffer
            mydata = (*++myi)->m_data;
            mycount = (*myi)->m_used;
        }
        if (rcount -= bytes) {
            rdata += bytes;
        } else {
            // advance to next remote buffer
            rdata = (*++ri)->m_data;
            rcount = (*ri)->m_used;
        }
    }
}

//===========================================================================
size_t CharBuf::copy(char * out, size_t count, size_t pos) const {
    assert(pos <= pos + count && pos + count <= size());
    auto ic = const_cast<CharBuf *>(this)->find(pos);
    Buffer * pbuf = *ic.first;
    char * ptr = pbuf->m_data + ic.second;
    size_t used = pbuf->m_used - ic.second;
    size_t remaining = count;
    for (;;) {
        size_t num = min(used, remaining);
        memcpy(out, ptr, num);
        remaining -= num;
        if (!remaining)
            break;
        pbuf = *++ic.first;
        ptr = pbuf->m_data;
        used = pbuf->m_used;
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
// static
CharBuf::Buffer * CharBuf::allocBuffer() {
    return allocBuffer(kDefaultBlockSize);
}

//===========================================================================
// static
CharBuf::Buffer * CharBuf::allocBuffer(size_t reserve) {
    size_t bytes =
        max(sizeof(Buffer),
            offsetof(Buffer, m_data) + reserve * sizeof(*Buffer::m_data));
    assert(bytes >= sizeof(Buffer));
    void * vptr = malloc(bytes);
    auto ptr = new (vptr) Buffer;
    ptr->m_reserved = (int)reserve;
    return ptr;
}

//===========================================================================
pair<vector<CharBuf::Buffer *>::const_iterator, int>
CharBuf::find(size_t pos) const {
    auto ic = const_cast<CharBuf *>(this)->find(pos);
    return ic;
}

//===========================================================================
pair<vector<CharBuf::Buffer *>::iterator, int> CharBuf::find(size_t pos) {
    int off = (int)pos;
    if (off < m_size / 2) {
        auto it = m_buffers.begin();
        for (;;) {
            int used = (*it)->m_used;
            if (off <= used) {
                if (off < used || used < (*it)->m_reserved)
                    return make_pair(it, off);
            }
            off -= used;
            ++it;
        }
    } else {
        assert(pos <= m_size);
        if (!m_size)
            m_buffers.emplace_back(allocBuffer());
        int base = m_size;
        auto it = m_buffers.end();
        for (;;) {
            --it;
            base -= (*it)->m_used;
            if (base <= off)
                return make_pair(it, off - base);
        }
    }
}

//===========================================================================
// Move the data (if any) after the split point to a new block immediately 
// following the block being split.
vector<CharBuf::Buffer *>::iterator CharBuf::split(
    vector<Buffer *>::iterator it, 
    int pos
) {
    assert(pos <= (*it)->m_used);
    auto rdata = (*it)->m_data + pos;
    auto rcount = (*it)->m_used - pos;
    int nbufs = 0;
    while (rcount) {
        nbufs += 1;
        it = m_buffers.emplace(++it, allocBuffer());
        auto mydata = (*it)->m_data;
        auto mycount = (*it)->m_reserved;
        int bytes = min(mycount, rcount);
        memcpy(mydata, rdata, bytes);
        (*it)->m_used = bytes;
        rcount -= bytes;
    }
    it -= nbufs;
    (*it)->m_used = pos;
    return it;
}

//===========================================================================
CharBuf & CharBuf::insert(
    vector<Buffer *>::iterator it,
    int pos,
    size_t numCh,
    char ch) {

    Buffer * pbuf = *it;
    char * ptr = pbuf->m_data + pos;
    int count = (int)numCh;
    m_size += count;

    int copied = pbuf->m_used - pos;
    if (count <= pbuf->m_reserved - pbuf->m_used) {
        // fits inside the current buffer
        if (count) {
            memmove(ptr + count, ptr, copied);
            memset(ptr, ch, count);
            pbuf->m_used += count;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    pbuf = *it;
    ptr = pbuf->m_data + pos;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before
    // the new block with the data that was after the insertion point.
    for (;;) {
        int added = min(count, int(pbuf->m_reserved - pos));
        memset(ptr, ch, added);
        pbuf->m_used += added;
        count -= added;
        if (!count)
            return *this;
        it = m_buffers.emplace(++it, allocBuffer());
        pbuf = *it;
        pos = 0;
        ptr = pbuf->m_data;
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    vector<CharBuf::Buffer *>::iterator it,
    int pos,
    const char s[]) {

    Buffer * pbuf = *it;
    char * ptr = pbuf->m_data + pos;

    // memchr s for \0 within the number of bytes that will fit in the
    // current block
    int copied = pbuf->m_used - pos;
    char * eptr = (char *)memchr(s, 0, pbuf->m_reserved - pbuf->m_used + 1);
    if (eptr) {
        // the string fits inside the current buffer
        int slen = int(eptr - s);
        memmove(ptr + slen, ptr, copied);
        memcpy(ptr, s, slen);
        pbuf->m_used += slen;
        m_size += slen;
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    pbuf = *it;
    ptr = pbuf->m_data + pos;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before
    // the new block with the data that was after the insertion point.
    char * base = ptr;
    eptr = pbuf->end();
    for (;;) {
        *ptr++ = *s++;
        if (!*s) {
            int added = int(ptr - base);
            pbuf->m_used += added;
            m_size += added;
            return *this;
        }
        if (ptr == eptr) {
            int added = int(ptr - base);
            pbuf->m_used += added;
            m_size += added;
            it = m_buffers.emplace(++it, allocBuffer());
            pbuf = *it;
            ptr = pbuf->m_data;
            base = ptr;
            eptr = pbuf->end();
        }
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    vector<Buffer *>::iterator it,
    int pos,
    const char s[],
    size_t slen) {

    Buffer * pbuf = *it;
    char * ptr = pbuf->m_data + pos;
    int count = (int)slen;
    m_size += count;

    int copied = pbuf->m_used - pos;
    if (count <= pbuf->m_reserved - pbuf->m_used) {
        // the string fits inside the current buffer
        if (count) {
            memmove(ptr + count, ptr, copied);
            memcpy(ptr, s, count);
            pbuf->m_used += count;
        }
        return *this;
    }

    // Split the block if we're inserting into the middle of it
    it = split(it, pos);
    pbuf = *it;
    ptr = pbuf->m_data + pos;

    // Copy to the rest of the current block and to as many new blocks as
    // needed. These blocks are inserted after the current block and before
    // the new block with the data that was after the insertion point.
    for (;;) {
        int added = min(count, int(pbuf->m_reserved - pos));
        memcpy(ptr, s, added);
        s += added;
        pbuf->m_used += added;
        count -= added;
        if (!count)
            return *this;
        it = m_buffers.emplace(++it, allocBuffer());
        pbuf = *it;
        pos = 0;
        ptr = pbuf->m_data;
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    vector<Buffer *>::iterator it,
    int pos,
    vector<Buffer *>::const_iterator srcIt,
    int srcPos,
    size_t srcLen
) {
    assert(0);
    return *this;
}

//===========================================================================
CharBuf &
CharBuf::erase(vector<CharBuf::Buffer *>::iterator it, int pos, int remove) {
    Buffer * pbuf = *it;
    assert(pos <= pbuf->m_used && pos >= 0);
    assert(m_size >= remove && remove >= 0);
    if (!remove)
        return *this;

    m_size -= remove;
    if (pos) {
        int copied = pbuf->m_used - pos - remove;
        if (copied > 0) {
            char * ptr = pbuf->m_data + pos;
            char * eptr = ptr + remove;
            memmove(ptr, eptr, copied);
            pbuf->m_used -= remove;
            return *this;
        }
        int removed = pbuf->m_used - pos;
        pbuf->m_used -= removed;
        remove -= removed;
        pbuf = *++it;
    }

    if (!remove)
        return *this;

    auto next = it;
    auto eb = m_buffers.end();
    while (pbuf->m_used <= remove) {
        ++next;
        remove -= pbuf->m_used;
        delete pbuf;
        m_buffers.erase(it);
        it = next;
        if (it == eb) {
            assert(!remove);
            return *this;
        }
        pbuf = *it;
    }

    pbuf->m_used -= remove;
    char * ptr = pbuf->m_data;
    char * eptr = ptr + remove;
    memmove(ptr, eptr, pbuf->m_used);
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
