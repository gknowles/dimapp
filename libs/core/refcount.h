// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// refcount.h - dim core
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {


/****************************************************************************
*
*   IRefCount
*
***/

class IRefCount {
public:
    struct Deleter {
        void operator()(IRefCount * ptr) { ptr->decRef(); }
    };

public:
    virtual ~IRefCount() = default;
    virtual void incRef() = 0;
    virtual void decRef() = 0;
    virtual void onZeroRef() = 0;
};

//===========================================================================
inline void intrusive_ptr_add_ref(IRefCount * ptr) {
    ptr->incRef();
}

//===========================================================================
inline void intrusive_ptr_release(IRefCount * ptr) {
    ptr->decRef();
}


/****************************************************************************
*
*   RefCount
*
***/

class RefCount : public IRefCount {
public:
    void incRef() override;
    void decRef() override;
    void onZeroRef() override;

private:
    unsigned m_refCount{0};
};

//===========================================================================
inline void RefCount::incRef() {
    m_refCount += 1;
}

//===========================================================================
inline void RefCount::decRef() {
    if (!--m_refCount)
        onZeroRef();
}

//===========================================================================
inline void RefCount::onZeroRef() {
    delete this;
}


/****************************************************************************
*
*   RefPtr
*
***/

template<typename T>
class RefPtr {
public:
    RefPtr() {}
    RefPtr(std::nullptr_t) {}
    explicit RefPtr(T * ptr);

    ~RefPtr();
    RefPtr & operator= (const RefPtr & ptr);

    void reset();
    void reset(T * ptr);
    void swap(RefPtr & other);
    T * release();

    explicit operator bool() const { return m_ptr; }
    T * get() { return m_ptr; }
    const T * get() const { return m_ptr; }

    T & operator* () { return *m_ptr; }
    T * operator-> () { return m_ptr; }
    const T & operator* () const { return *m_ptr; }
    const T * operator-> () const { return m_ptr; }

private:
    T * m_ptr{nullptr};
};

//===========================================================================
template<typename T>
RefPtr<T>::RefPtr(T * ptr) {
    reset(ptr);
}

//===========================================================================
template<typename T>
RefPtr<T>::~RefPtr() {
    if (m_ptr)
        m_ptr->decRef();
}

//===========================================================================
template<typename T>
RefPtr<T> & RefPtr<T>::operator= (const RefPtr & ptr) {
    m_ptr = ptr->m_ptr;
    if (m_ptr)
        m_ptr->incRef();
    return *this;
}

//===========================================================================
template<typename T>
void RefPtr<T>::reset() {
    if (m_ptr)
        m_ptr->decRef();
    m_ptr = nullptr;
}

//===========================================================================
template<typename T>
void RefPtr<T>::reset(T * ptr) {
    if (ptr)
        ptr->incRef();
    if (m_ptr)
        m_ptr->decRef();
    m_ptr = ptr;
}

//===========================================================================
template<typename T>
T * RefPtr<T>::release() {
    auto ptr = m_ptr;
    m_ptr = nullptr;
    return ptr;
}

//===========================================================================
template<typename T>
void RefPtr<T>::swap(RefPtr & other) {
    ::swap(m_ptr, other.m_ptr);
}

} // namespace
