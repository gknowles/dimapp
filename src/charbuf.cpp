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
CharBuf & CharBuf::assign(const char s[]) {
    return replace(0, m_size, s);
}

//===========================================================================
CharBuf & CharBuf::assign(const char s[], size_t count) {
    return replace(0, m_size, s, count);
}

//===========================================================================
CharBuf & CharBuf::assign(const string & str, size_t pos, size_t count) {
    assert(pos < str.size());
    return replace(
        0, m_size, str.data() + pos, min(str.size(), pos + count) - pos);
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
CharBuf & CharBuf::insert(size_t pos, const char s[]) {
    assert(pos <= m_size);
    if (!*s)
        return *this;
    auto ic = find(pos);
    return insert(ic.first, ic.second, s);
}

//===========================================================================
CharBuf & CharBuf::insert(size_t pos, const char s[], size_t count) {
    return replace(pos, 0, s, count);
}

//===========================================================================
CharBuf & CharBuf::erase(size_t pos, size_t count) {
    assert(pos + count > pos && pos + count <= m_size);
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
    append(1, ch);
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
    assert(m_size + count < numeric_limits<int>::max());
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
    if (!*s)
        return *this;

    if (!m_size)
        m_buffers.emplace_back(allocBuffer());

    for (;;) {
        Buffer * buf = m_buffers.back();
        char * ptr = buf->unused();
        char * eptr = buf->end();
        for (;;) {
            if (ptr == eptr) {
                m_size += buf->m_reserved - buf->m_used;
                buf->m_used = buf->m_reserved;
                m_buffers.emplace_back(allocBuffer());
                break;
            }
            *ptr++ = *s++;
            if (!*s) {
                int added = int(ptr - buf->m_data) - buf->m_used;
                m_size += added;
                buf->m_used += added;
                return *this;
            }
        }
    }
}

//===========================================================================
CharBuf & CharBuf::append(const char src[], size_t srcLen) {
    assert(m_size + srcLen < numeric_limits<int>::max());
    int add = (int)srcLen;
    if (!add)
        return *this;

    if (!m_size)
        m_buffers.emplace_back(allocBuffer());
    m_size += add;

    for (;;) {
        Buffer * buf = m_buffers.back();
        int added = min(add, buf->m_reserved - buf->m_used);
        memcpy(buf->unused(), src, added);
        buf->m_used += added;
        add -= added;
        if (!add)
            return *this;
        m_buffers.emplace_back(allocBuffer());
    }
}

//===========================================================================
CharBuf & CharBuf::append(const string & str, size_t pos, size_t count) {
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
    for (auto && ptr : m_buffers) {
        if (count < ptr->m_used) {
            if (memcmp(ptr->m_data, s, count) < 0)
                return -1;
            return 1;
        }
        if (int rc = memcmp(ptr->m_data, s, ptr->m_used))
            return rc;
        s += ptr->m_used;
        count -= ptr->m_used;
    }
    return count ? -1 : 0;
}

//===========================================================================
int CharBuf::compare(const string & str) const {
    return compare(str.data(), str.size());
}

//===========================================================================
int CharBuf::compare(const CharBuf & buf) const {
    auto myi = m_buffers.begin();
    auto mye = m_buffers.end();
    const char * mydata;
    int mycount;
    auto ri = buf.m_buffers.begin();
    auto re = buf.m_buffers.end();
    const char * rdata;
    int rcount;
    goto compare_new_buffers;

    for (;;) {
        if (mycount < rcount) {
            int rc = memcmp(mydata, rdata, mycount);
            if (rc)
                return rc;
            if (++myi == mye)
                return -1;
            rdata += mycount;
            rcount -= mycount;
            mydata = (*myi)->m_data;
            mycount = (*myi)->m_used;
            continue;
        }

        int rc = memcmp(mydata, rdata, rcount);
        if (rc)
            return rc;
        if (mycount > rcount) {
            if (++ri == re)
                return 1;
            mydata += rcount;
            mycount -= rcount;
            rdata = (*ri)->m_data;
            rcount = (*ri)->m_used;
            continue;
        }
        ++myi;
        ++ri;

    compare_new_buffers:
        if (myi == mye)
            return (ri == re) ? 0 : -1;
        if (ri == re)
            return 1;
        mydata = (*myi)->m_data;
        mycount = (*myi)->m_used;
        rdata = (*ri)->m_data;
        rcount = (*ri)->m_used;
    }
}

//===========================================================================
CharBuf & CharBuf::replace(size_t pos, size_t count, const char s[]) {
    assert(pos + count <= m_size);

    if (!count)
        return insert(pos, s);

    int remove = (int)count;
    auto ic = find(pos);

    // copy the overlap, then either erase or insert the rest
    Buffer * pbuf = *ic.first;
    char * base = pbuf->m_data + ic.second;
    char * ptr = base;
    int copied = min(pbuf->m_used - ic.second, remove);
    char * eptr = ptr + copied;
    for (;;) {
        if (ptr == eptr) {
            if (!*s)
                return *this;
            remove -= copied;
            if (!remove) 
                return insert(ic.first, int(ptr - pbuf->m_data), s);
           ++ic.first;
            pbuf = *ic.first;
            base = pbuf->m_data;
            ptr = base;
            copied = min(pbuf->m_used, remove);
            eptr = ptr + copied;
        } else if (!*s) {
            return erase(
                ic.first, int(ptr - pbuf->m_data), remove - int(ptr - base));
        }
        *ptr++ = *s++;
    }
}

//===========================================================================
CharBuf &
CharBuf::replace(size_t pos, size_t count, const char src[], size_t srcLen) {
    assert(pos + count <= m_size);
    if (pos == m_size)
        return append(src, srcLen);

    int remove = (int)count;
    int copy = (int)srcLen;
    auto ic = find(pos);
    auto eb = m_buffers.end();
    Buffer * pbuf = *ic.first;
    m_size += copy - remove;

    char * ptr = pbuf->m_data + ic.second;
    int reserved = pbuf->m_reserved - ic.second;
    int used = pbuf->m_used - ic.second;
    int replaced;
    for (;;) {
        if (!copy) {
            return erase(ic.first, int(ptr - pbuf->m_data), remove);
        }

        replaced = min({reserved, remove, copy});
        memcpy(ptr, src, replaced);
        src += replaced;
        copy -= replaced;
        remove -= min(used, replaced);
        if (!remove)
            break;
        ic.second = 0;
        ++ic.first;
        assert(ic.first != eb);
        pbuf = *ic.first;
        ptr = pbuf->m_data;
        reserved = pbuf->m_reserved;
        used = pbuf->m_used;
    }

    if (copy + used <= reserved) {
        ptr += replaced;
        if (used > replaced)
            memmove(ptr + copy, ptr, used - replaced);
        memcpy(ptr, src, copy);
        pbuf->m_used += copy;
        return *this;
    }

    auto next = ic.first;
    ++next;

    // shift following to new block
    if (used > replaced) {
        Buffer * tmp = *m_buffers.emplace(next, allocBuffer());
        memcpy(tmp->m_data, ptr, used - replaced);
        tmp->m_used = used - replaced;
    }
    // copy remaining source into new blocks
    for (;;) {
        memcpy(ptr, src, replaced);
        pbuf->m_used += replaced;
        src += replaced;
        copy -= replaced;
        if (!copy)
            return *this;
        ic.first = m_buffers.emplace(next, allocBuffer());
        pbuf = *ic.first;
        ptr = pbuf->m_data;
        replaced = min(pbuf->m_reserved, copy);
    }
}

//===========================================================================
size_t CharBuf::copy(char * out, size_t count, size_t pos) const {
    assert(pos + count <= size());
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
pair<vector<CharBuf::Buffer *>::iterator, int> CharBuf::find(size_t pos) {
    int off = (int)pos;
    if (off <= m_size / 2) {
        if (!m_size)
            m_buffers.emplace_back(allocBuffer());
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
        int base = m_size;
        auto it = m_buffers.end();
        if (m_buffers.empty()) {
            assert(pos == 0);
            return make_pair(it, off);
        }
        for (;;) {
            --it;
            base -= (*it)->m_used;
            if (base <= off)
                return make_pair(it, off - base);
        }
    }
}

//===========================================================================
CharBuf & CharBuf::insert(
    std::vector<CharBuf::Buffer *>::iterator it, int pos, const char s[]) {

    Buffer * pbuf = *it;
    char * ptr = pbuf->m_data + pos;

    // memchr s for \0 within the number of bytes that will fit in the 
    // current block
    int copied = pbuf->m_used - pos;
    char * eptr = (char *) memchr(s, 0, pbuf->m_reserved - pbuf->m_used + 1);
    if (eptr) {
        // the string fits inside the current buffer
        int slen = int(eptr - s);
        memmove(ptr + slen, ptr, copied);
        memcpy(ptr, s, slen);
        pbuf->m_used += slen;
        m_size += slen;
        return *this;
    }

    // The string doesn't fit in the current block, move the data (if any)
    // after the insertion point in the current block to a new block 
    // immediately following the current one. 
    if (copied) {
        it = m_buffers.emplace(++it, allocBuffer());
        auto nextbuf = *it--;
        char * dst = nextbuf->m_data;
        memcpy(dst, ptr, copied);
        nextbuf->m_used = copied;
    }

    // Copy s to the rest of the current block and to as many new blocks as 
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
bool Dim::operator==(const CharBuf & left, const std::string & right) {
    return left.compare(right) == 0;
}

//===========================================================================
bool Dim::operator==(const std::string & left, const CharBuf & right) {
    return right.compare(left) == 0;
}

//===========================================================================
bool Dim::operator==(const CharBuf & left, const CharBuf & right) {
    return left.compare(right) == 0;
}

//===========================================================================
std::string Dim::to_string(const CharBuf & buf) {
    string out;
    out.resize(buf.size());
    buf.copy(out.data(), buf.size());
    return out;
}
