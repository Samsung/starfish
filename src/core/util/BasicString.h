/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishBasicString__
#define __StarFishBasicString__

namespace StarFish {

template <typename T, typename Allocator>
class BasicString : public gc {
protected:
    typedef typename Allocator::template rebind<T>::other TAllocType;

    // Types:
public:
    typedef typename std::char_traits<T> traits_type;
    typedef typename TAllocType::size_type size_type;
    typedef typename TAllocType::difference_type difference_type;
    typedef typename TAllocType::pointer pointer;
    typedef typename TAllocType::const_pointer const_pointer;
    typedef __gnu_cxx::__normal_iterator<pointer, BasicString> iterator;
    typedef __gnu_cxx::__normal_iterator<const_pointer, BasicString>
        const_iterator;

    BasicString()
    {
        makeEmpty();
    }

    BasicString(const T* src)
    {
        if (src) {
            size_t len = traits_type::length(src);
            copy(src, len, len + 1);
        } else {
            makeEmpty();
        }
    }

    BasicString(const T* src, size_t len)
    {
        if (src) {
            copy(src, len, len + 1);
        } else {
            makeEmpty();
        }
    }

    BasicString(BasicString<T, Allocator>&& other)
        : m_buffer(other.m_buffer)
        , m_size(other.m_size)
    {
        other.makeEmpty();
    }

    BasicString(const BasicString<T, Allocator>& other)
    {
        if (other.m_size > 0) {
            copy(other, other.m_size + 1);
        } else {
            makeEmpty();
        }
    }

    const BasicString<T, Allocator>& operator=(
        const BasicString<T, Allocator>& other)
    {
        if (other.m_size > 0) {
            copy(other, other.m_size + 1);
        } else {
            clear();
        }
        return *this;
    }

    ~BasicString()
    {
        deallocate();
    }

    void push_back(T val)
    {
        pushBack(val);
    }

    const BasicString<T, Allocator> operator+(
        const BasicString<T, Allocator>& other)
    {
        BasicString ret(*this);
        ret.append(other);
        return ret;
    }

    const BasicString<T, Allocator>& operator+=(
        const BasicString<T, Allocator>& other)
    {
        append(other);
        return *this;
    }

    const BasicString<T, Allocator>& operator+=(T other)
    {
        pushBack(other);
        return *this;
    }

    void append(const BasicString& other)
    {
        append(other.data(), traits_type::length(other.data()));
    }

    void append(const T* src)
    {
        append(src, traits_type::length(src));
    }

    void append(const T* src, size_t len);

    void insert(size_t pos, const T val);

    void erase(size_t pos)
    {
        erase(pos, pos + 1);
    }

    void erase(size_t start, size_t end);

    BasicString<T, Allocator>& replace(size_t start, size_t len,
                                       BasicString<T, Allocator>& dst);

    BasicString<T, Allocator> substr(size_t pos, size_t len) const;

    size_t find(T c, size_t pos = 0) const;

    size_t find(const BasicString<T, Allocator>& src, size_t pos = 0) const
    {
        return find(src.data(), pos, src.length());
    }

    size_t find(const T* src, size_t pos = 0) const
    {
        return find(src, pos, traits_type::length(src));
    }

    size_t find(const BasicString<T, Allocator>& src, size_t pos,
                size_t n) const
    {
        return find(src.data(), pos, n);
    }

    size_t find(const T* src, size_t pos, size_t n) const;

    size_t size() const
    {
        return m_size;
    }

    size_t length() const
    {
        return m_size;
    }

    size_t capacity() const
    {
        if (m_buffer) {
            return m_size + 1;
        } else {
            return 0;
        }
    }

    iterator begin()
    {
        return iterator(data());
    }

    const_iterator begin() const
    {
        return const_iterator(data());
    }

    iterator end()
    {
        return iterator(data() + m_size);
    }

    const_iterator end() const
    {
        return const_iterator(data() + m_size);
    }

    bool empty() const
    {
        return m_size == 0;
    }

    void pop_back()
    {
        erase(m_size - 1);
    }

    T& operator[](const size_t& idx)
    {
        STARFISH_ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    const T& operator[](const size_t& idx) const
    {
        STARFISH_ASSERT(idx < m_size);
        return m_buffer[idx];
    }

    void clear()
    {
        deallocate();
        makeEmpty();
    }

    T* data() const
    {
        STARFISH_ASSERT(m_buffer != nullptr);
        return m_buffer;
    }

    void resize(size_t newSize)
    {
        if (newSize == 0) {
            clear();
        } else {
            _resize(newSize, true);
        }
    }

    void reserve(size_t newSize)
    {
        if (newSize == 0) {
            clear();
        } else {
            _resize(newSize, false);
        }
    }

    int compare(const BasicString<T, Allocator>& other) const
    {
        const size_t osize = other.size();
        const size_t len = std::min(m_size, osize);

        int r = traits_type::compare(data(), other.data(), len);
        if (!r) {
            r = _compare(m_size, osize);
        }
        return r;
    }

protected:
    void makeEmpty()
    {
        m_buffer = allocate(1);
        m_size = 0;
    }

    T* allocate(size_t size) const;

    void copy(const T* src, size_t size, size_t capacity)
    {
        STARFISH_ASSERT(src);
        m_buffer = allocate(capacity);
        m_size = size;
        memcpy(m_buffer, src, sizeof(T) * m_size);
        m_buffer[m_size] = 0;
    }

    void copy(const BasicString<T, Allocator>& other, size_t capacity)
    {
        m_size = other.m_size;
        m_buffer = allocate(capacity);
        memcpy(m_buffer, other.data(), sizeof(T) * m_size);
        m_buffer[m_size] = 0;
    }

    void _resize(size_t newSize, bool init)
    {
        T* newBuffer = allocate(newSize + 1);
        size_t len = std::min(m_size, newSize);
        memcpy(newBuffer, m_buffer, len * sizeof(T));
        if (init && newSize > len) {
            memset(newBuffer + len, 0, (newSize - len + 1) * sizeof(T));
        } else {
            newBuffer[len] = 0;
        }

        deallocate();
        if (init) {
            m_size = newSize;
        } else {
            m_size = len;
        }
        m_buffer = newBuffer;
    }

    void pushBack(T val);

    std::tuple<T*, size_t> _substr(size_t pos, size_t len) const;

    // Important! `m_size` update should follow `deallocate()`
    void deallocate()
    {
        if (m_buffer) {
            STARFISH_ASSERT(capacity() > 0);
            Allocator().deallocate(m_buffer, capacity());
            m_buffer = nullptr;
            m_size = 0;
        }
    }

    static int _compare(size_t n1, size_t n2)
    {
        const difference_type d = difference_type(n1 - n2);

        if (d > __gnu_cxx::__numeric_traits<int>::__max) {
            return __gnu_cxx::__numeric_traits<int>::__max;
        } else if (d < __gnu_cxx::__numeric_traits<int>::__min) {
            return __gnu_cxx::__numeric_traits<int>::__min;
        } else {
            return int(d);
        }
    }

    T* m_buffer;
    size_t m_size;
};
}

#endif

#include "core/util/BasicString.hpp"
