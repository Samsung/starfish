/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// TODO add RefPtr orginal License comment

#ifndef __StarFishRefPtr__
#define __StarFishRefPtr__

#include "core/util/RefCounted.h"

namespace StarFish {

template <typename T>
class PassRef;
template <typename T>
class PassRefPtr;
template <typename T>
class Ref;
template <typename T>
class RefPtr;

template <typename T>
PassRef<T> adoptRef(T&);

inline void adopted(const void*)
{
}

template <typename T>
class PassRef {
public:
    PassRef(T&);
    PassRef(PassRef&&);
    template <typename U>
    PassRef(PassRef<U>);

    const T& get() const;
    T& get();

    void dropRef();
    T& leakRef() WARN_UNUSED_RETURN;

#ifndef NDEBUG
    ~PassRef();
#endif

private:
    friend PassRef adoptRef<T>(T&);

    template <typename U>
    friend class PassRef;
    template <typename U>
    friend class PassRefPtr;
    template <typename U>
    friend class Ref;
    template <typename U>
    friend class RefPtr;

    enum AdoptTag { Adopt };
    PassRef(T&, AdoptTag);

    T& m_reference;

#ifndef NDEBUG
    bool m_gaveUpReference;
#endif
};

template <typename T>
inline PassRef<T>::PassRef(T& reference)
    : m_reference(reference)
#ifndef NDEBUG
    , m_gaveUpReference(false)
#endif
{
    reference.ref();
}

template <typename T>
inline PassRef<T>::PassRef(PassRef&& other)
    : m_reference(other.leakRef())
#ifndef NDEBUG
    , m_gaveUpReference(false)
#endif
{
}

template <typename T>
template <typename U>
inline PassRef<T>::PassRef(PassRef<U> other)
    : m_reference(other.leakRef())
#ifndef NDEBUG
    , m_gaveUpReference(false)
#endif
{
}

#ifndef NDEBUG

template <typename T>
PassRef<T>::~PassRef()
{
    STARFISH_ASSERT(m_gaveUpReference);
}

#endif

template <typename T>
inline void PassRef<T>::dropRef()
{
#ifndef NDEBUG
    STARFISH_ASSERT(!m_gaveUpReference);
#endif
    m_reference.deref();
#ifndef NDEBUG
    m_gaveUpReference = true;
#endif
}

template <typename T>
inline const T& PassRef<T>::get() const
{
#ifndef NDEBUG
    STARFISH_ASSERT(!m_gaveUpReference);
#endif
    return m_reference;
}

template <typename T>
inline T& PassRef<T>::get()
{
#ifndef NDEBUG
    STARFISH_ASSERT(!m_gaveUpReference);
#endif
    return m_reference;
}

template <typename T>
inline T& PassRef<T>::leakRef()
{
#ifndef NDEBUG
    STARFISH_ASSERT(!m_gaveUpReference);
    m_gaveUpReference = true;
#endif
    return m_reference;
}

template <typename T>
inline PassRef<T>::PassRef(T& reference, AdoptTag)
    : m_reference(reference)
#ifndef NDEBUG
    , m_gaveUpReference(false)
#endif
{
}

template <typename T>
inline PassRef<T> adoptRef(T& reference)
{
    adopted(&reference);
    return PassRef<T>(reference, PassRef<T>::Adopt);
}

template <typename T, typename... Args>
inline PassRef<T> createRefCounted(Args&&... args)
{
    return adoptRef(*new T(std::forward<Args>(args)...));
}

template <typename T>
PassRefPtr<T> adoptRef(T*);

template <typename T>
ALWAYS_INLINE void refIfNotNull(T* ptr)
{
    if (LIKELY(ptr != nullptr))
        ptr->ref();
}

template <typename T>
ALWAYS_INLINE void derefIfNotNull(T* ptr)
{
    if (LIKELY(ptr != nullptr))
        ptr->deref();
}

template <typename T>
class PassRefPtr {
public:
    PassRefPtr()
        : m_ptr(nullptr)
    {
    }
    PassRefPtr(T* ptr)
        : m_ptr(ptr)
    {
        refIfNotNull(ptr);
    }
    // It somewhat breaks the type system to allow transfer of ownership out of
    // a const PassRefPtr. However, it makes it much easier to work with
    // PassRefPtr
    // temporaries, and we don't have a need to use real const PassRefPtrs
    // anyway.
    PassRefPtr(const PassRefPtr& o)
        : m_ptr(o.leakRef())
    {
    }
    template <typename U>
    PassRefPtr(const PassRefPtr<U>& o)
        : m_ptr(o.leakRef())
    {
    }

    ALWAYS_INLINE ~PassRefPtr()
    {
        derefIfNotNull(m_ptr);
    }

    template <typename U>
    PassRefPtr(const RefPtr<U>&);
    template <typename U>
    PassRefPtr(PassRef<U> reference)
        : m_ptr(&reference.leakRef())
    {
    }

    T* get() const
    {
        return m_ptr;
    }

    T* leakRef() const WARN_UNUSED_RETURN;

    T& operator*() const
    {
        return *m_ptr;
    }
    T* operator->() const
    {
        return m_ptr;
    }

    bool operator!() const
    {
        return !m_ptr;
    }

    // This conversion operator allows implicit conversion to bool but not to
    // other integer types.
    typedef T*(PassRefPtr::*UnspecifiedBoolType);
    operator UnspecifiedBoolType() const
    {
        return m_ptr ? &PassRefPtr::m_ptr : nullptr;
    }

    friend PassRefPtr adoptRef<T>(T*);

private:
    PassRefPtr& operator=(const PassRefPtr&) = delete;

    enum AdoptTag { Adopt };
    PassRefPtr(T* ptr, AdoptTag)
        : m_ptr(ptr)
    {
    }

    mutable T* m_ptr;
};

template <typename T>
template <typename U>
inline PassRefPtr<T>::PassRefPtr(const RefPtr<U>& o)
    : m_ptr(o.get())
{
    T* ptr = m_ptr;
    refIfNotNull(ptr);
}

template <typename T>
inline T* PassRefPtr<T>::leakRef() const
{
    T* ptr = m_ptr;
    m_ptr = nullptr;
    return ptr;
}

template <typename T, typename U>
inline bool operator==(const PassRefPtr<T>& a, const PassRefPtr<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator==(const PassRefPtr<T>& a, const RefPtr<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator==(const RefPtr<T>& a, const PassRefPtr<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator==(const PassRefPtr<T>& a, U* b)
{
    return a.get() == b;
}

template <typename T, typename U>
inline bool operator==(T* a, const PassRefPtr<U>& b)
{
    return a == b.get();
}

template <typename T, typename U>
inline bool operator!=(const PassRefPtr<T>& a, const PassRefPtr<U>& b)
{
    return a.get() != b.get();
}

template <typename T, typename U>
inline bool operator!=(const PassRefPtr<T>& a, const RefPtr<U>& b)
{
    return a.get() != b.get();
}

template <typename T, typename U>
inline bool operator!=(const RefPtr<T>& a, const PassRefPtr<U>& b)
{
    return a.get() != b.get();
}

template <typename T, typename U>
inline bool operator!=(const PassRefPtr<T>& a, U* b)
{
    return a.get() != b;
}

template <typename T, typename U>
inline bool operator!=(T* a, const PassRefPtr<U>& b)
{
    return a != b.get();
}

template <typename T>
inline PassRefPtr<T> adoptRef(T* p)
{
    adopted(p);
    return PassRefPtr<T>(p, PassRefPtr<T>::Adopt);
}

template <typename T, typename U>
inline PassRefPtr<T> static_pointer_cast(const PassRefPtr<U>& p)
{
    return adoptRef(static_cast<T*>(p.leakRef()));
}

template <typename T>
inline T* getPtr(const PassRefPtr<T>& p)
{
    return p.get();
}

template <typename T>
class Ref {
    T& val;

public:
    Ref(T& val)
        : val(val)
    {
    }
    operator T&() const
    {
        return val;
    }
};

enum HashTableDeletedValueType { HashTableDeletedValue };

template <typename T>
class RefPtr {
public:
    ALWAYS_INLINE RefPtr()
        : m_ptr(nullptr)
    {
    }
    ALWAYS_INLINE RefPtr(T* ptr)
        : m_ptr(ptr)
    {
        refIfNotNull(ptr);
    }
    ALWAYS_INLINE RefPtr(const RefPtr& o)
        : m_ptr(o.m_ptr)
    {
        refIfNotNull(m_ptr);
    }
    template <typename U>
    RefPtr(const RefPtr<U>& o)
        : m_ptr(o.get())
    {
        refIfNotNull(m_ptr);
    }

    ALWAYS_INLINE RefPtr(RefPtr&& o)
        : m_ptr(o.release().leakRef())
    {
    }
    template <typename U>
    RefPtr(RefPtr<U>&& o)
        : m_ptr(o.release().leakRef())
    {
    }

    // See comments in PassRefPtr.h for an explanation of why this takes a const
    // reference.
    template <typename U>
    RefPtr(const PassRefPtr<U>&);

    template <typename U>
    RefPtr(PassRef<U>);

    // Hash table deleted values, which are only constructed and never copied or
    // destroyed.
    RefPtr(HashTableDeletedValueType)
        : m_ptr(hashTableDeletedValue())
    {
    }
    bool isHashTableDeletedValue() const
    {
        return m_ptr == hashTableDeletedValue();
    }

    ALWAYS_INLINE ~RefPtr()
    {
        derefIfNotNull(m_ptr);
    }

    T* get() const
    {
        return m_ptr;
    }

    void clear();
    PassRefPtr<T> release()
    {
        PassRefPtr<T> tmp = adoptRef(m_ptr);
        m_ptr = nullptr;
        return tmp;
    }
    PassRef<T> releaseNonNull()
    {
        STARFISH_ASSERT(m_ptr);
        PassRef<T> tmp = adoptRef(*m_ptr);
        m_ptr = nullptr;
        return tmp;
    }

    T& operator*() const
    {
        return *m_ptr;
    }
    ALWAYS_INLINE T* operator->() const
    {
        return m_ptr;
    }

    bool operator!() const
    {
        return !m_ptr;
    }

    // This conversion operator allows implicit conversion to bool but not to
    // other integer types.
    typedef T*(RefPtr::*UnspecifiedBoolType);
    operator UnspecifiedBoolType() const
    {
        return m_ptr ? &RefPtr::m_ptr : nullptr;
    }

    RefPtr& operator=(const RefPtr&);
    RefPtr& operator=(T*);
    RefPtr& operator=(const PassRefPtr<T>&);
    template <typename U>
    RefPtr& operator=(const RefPtr<U>&);
    template <typename U>
    RefPtr& operator=(const PassRefPtr<U>&);
    RefPtr& operator=(RefPtr&&);
    template <typename U>
    RefPtr& operator=(RefPtr<U>&&);
    template <typename U>
    RefPtr& operator=(PassRef<U>);

    void swap(RefPtr&);

    static T* hashTableDeletedValue()
    {
        return reinterpret_cast<T*>(-1);
    }

private:
    T* m_ptr;
};

template <typename T>
template <typename U>
inline RefPtr<T>::RefPtr(const PassRefPtr<U>& o)
    : m_ptr(o.leakRef())
{
}

template <typename T>
template <typename U>
inline RefPtr<T>::RefPtr(PassRef<U> reference)
    : m_ptr(&reference.leakRef())
{
}

template <typename T>
inline void RefPtr<T>::clear()
{
    T* ptr = m_ptr;
    m_ptr = nullptr;
    derefIfNotNull(ptr);
}

template <typename T>
inline RefPtr<T>& RefPtr<T>::operator=(const RefPtr& o)
{
    RefPtr ptr = o;
    swap(ptr);
    return *this;
}

template <typename T>
template <typename U>
inline RefPtr<T>& RefPtr<T>::operator=(const RefPtr<U>& o)
{
    RefPtr ptr = o;
    swap(ptr);
    return *this;
}

template <typename T>
inline RefPtr<T>& RefPtr<T>::operator=(T* optr)
{
    RefPtr ptr = optr;
    swap(ptr);
    return *this;
}

template <typename T>
inline RefPtr<T>& RefPtr<T>::operator=(const PassRefPtr<T>& o)
{
    RefPtr ptr = o;
    swap(ptr);
    return *this;
}

template <typename T>
template <typename U>
inline RefPtr<T>& RefPtr<T>::operator=(const PassRefPtr<U>& o)
{
    RefPtr ptr = o;
    swap(ptr);
    return *this;
}

template <typename T>
inline RefPtr<T>& RefPtr<T>::operator=(RefPtr&& o)
{
    RefPtr ptr = std::move(o);
    swap(ptr);
    return *this;
}

template <typename T>
template <typename U>
inline RefPtr<T>& RefPtr<T>::operator=(RefPtr<U>&& o)
{
    RefPtr ptr = std::move(o);
    swap(ptr);
    return *this;
}

template <typename T>
template <typename U>
inline RefPtr<T>& RefPtr<T>::operator=(PassRef<U> reference)
{
    RefPtr ptr = std::move(reference);
    swap(ptr);
    return *this;
}

template <class T>
inline void RefPtr<T>::swap(RefPtr& o)
{
    std::swap(m_ptr, o.m_ptr);
}

template <class T>
inline void swap(RefPtr<T>& a, RefPtr<T>& b)
{
    a.swap(b);
}

template <typename T, typename U>
inline bool operator==(const RefPtr<T>& a, const RefPtr<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator==(const RefPtr<T>& a, U* b)
{
    return a.get() == b;
}

template <typename T, typename U>
inline bool operator==(T* a, const RefPtr<U>& b)
{
    return a == b.get();
}

template <typename T, typename U>
inline bool operator!=(const RefPtr<T>& a, const RefPtr<U>& b)
{
    return a.get() != b.get();
}

template <typename T, typename U>
inline bool operator!=(const RefPtr<T>& a, U* b)
{
    return a.get() != b;
}

template <typename T, typename U>
inline bool operator!=(T* a, const RefPtr<U>& b)
{
    return a != b.get();
}

template <typename T, typename U>
inline RefPtr<T> static_pointer_cast(const RefPtr<U>& p)
{
    return RefPtr<T>(static_cast<T*>(p.get()));
}

template <typename T>
inline T* getPtr(const RefPtr<T>& p)
{
    return p.get();
}
}
#endif
