// charbuf.h - dim services
#pragma once

#include "config.h"

#include "tempheap.h"

#include <list>
#include <string>
#include <string_view>
#include <utility> // std::pair
#include <vector>

namespace Dim {

class CharBuf : public ITempHeap {
public:
    CharBuf();
    CharBuf(CharBuf && from);
    ~CharBuf();

    CharBuf & operator=(char ch) { return assign(ch); }
    CharBuf & operator=(const char s[]) { return assign(s); }
    CharBuf & operator=(std::string_view str) { return assign(str); }
    CharBuf & operator=(const CharBuf & buf) { return assign(buf); }
    CharBuf & operator=(CharBuf && buf) { clear(); swap(buf); return *this; }
    CharBuf & operator+=(char ch) { pushBack(ch); return *this; }
    CharBuf & operator+=(const char s[]) { return append(s); }
    CharBuf & operator+=(std::string_view str) { return append(str); }
    CharBuf & operator+=(const CharBuf & src) { return append(src); }
    CharBuf & assign(char ch) { clear(); pushBack(ch); return *this; }
    CharBuf & assign(size_t numCh, char ch);
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
    void clear();
    CharBuf & insert(size_t pos, size_t numCh, char ch);
    CharBuf & insert(size_t pos, const char s[]);
    CharBuf & insert(size_t pos, const char s[], size_t count);
    CharBuf & insert(
        size_t pos, 
        const CharBuf & buf, 
        size_t bufPos = 0, 
        size_t bufLen = -1);
    CharBuf & erase(size_t pos = 0, size_t count = -1);
    CharBuf & ltrim(char ch);
    CharBuf & rtrim(char ch);
    void pushBack(char ch);
    void popBack();
    CharBuf & append(size_t numCh, char ch);
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
        size_t bufLen = -1) const;
    CharBuf & replace(size_t pos, size_t count, size_t numCh, char ch);
    CharBuf & replace(size_t pos, size_t count, const char s[]);
    CharBuf & replace(size_t pos, size_t count, const char s[], size_t slen);
    CharBuf & replace(
        size_t pos,
        size_t count,
        const CharBuf & src,
        size_t srcPos = 0,
        size_t srcLen = -1);
    size_t copy(char * out, size_t count, size_t pos = 0) const;
    void swap(CharBuf & other);

    // ITempHeap
    char * alloc(size_t bytes, size_t align) override;

private:
    struct Buffer;
    static Buffer * allocBuffer();
    static Buffer * allocBuffer(size_t reserve);
    std::pair<std::vector<Buffer *>::iterator, int> find(size_t pos);
    std::pair<std::vector<Buffer *>::const_iterator, int>
    find(size_t pos) const;
    std::vector<Buffer *>::iterator 
    split(std::vector<Buffer *>::iterator it, int pos);
    CharBuf &
    insert(std::vector<Buffer *>::iterator it, int pos, size_t numCh, char ch);
    CharBuf &
    insert(std::vector<Buffer *>::iterator it, int pos, const char src[]);
    CharBuf & insert(
        std::vector<Buffer *>::iterator it,
        int pos,
        const char src[],
        size_t srcLen);
    CharBuf & insert(
        std::vector<Buffer *>::iterator it,
        int pos,
        std::vector<Buffer *>::const_iterator srcIt,
        int srcPos,
        size_t srcLen);
    CharBuf & erase(std::vector<Buffer *>::iterator it, int pos, int count);

    std::vector<Buffer *> m_buffers;
    int m_lastUsed{0};
    int m_size{0};
};

bool operator==(const CharBuf & left, std::string_view right);
bool operator==(std::string_view left, const CharBuf & right);
bool operator==(const CharBuf & left, const CharBuf & right);

std::string to_string(const CharBuf & buf, size_t pos = 0, size_t count = -1);

} // namespace
