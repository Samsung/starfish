/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishBasicString__
#define __StarFishBasicString__

namespace StarFish {

template <typename T>
struct AllocInfo {
public:
    AllocInfo(T* buffer, size_t capacity)
        : m_buffer(buffer)
        , m_capacity(capacity)
    {
    }

    T* m_buffer;
    size_t m_capacity;
};

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
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    BasicString()
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        makeEmpty();
    }

    BasicString(const T* src)
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        if (src) {
            size_t len = traits_type::length(src);
            construct(src, len);
        } else {
            makeEmpty();
        }
    }

    BasicString(const T* src, size_t len)
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        if (src) {
            construct(src, len);
        } else {
            makeEmpty();
        }
    }

    BasicString(BasicString<T, Allocator>&& other)
        : m_buffer(other.m_buffer)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        other.m_buffer = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    BasicString(const BasicString<T, Allocator>& other)
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        if (other.m_size > 0) {
            construct(other);
        } else {
            makeEmpty();
        }
    }

    template <typename _InputIterator>
    BasicString(_InputIterator start, _InputIterator end)
        : m_buffer(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
        construct(start, end);
    }

    BasicString<T, Allocator>& operator=(const BasicString<T, Allocator>& other)
    {
        if (other.m_size > 0) {
            deallocate();
            construct(other);
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

    BasicString<T, Allocator>& operator+=(
        const BasicString<T, Allocator>& other)
    {
        append(other);
        return *this;
    }

    BasicString<T, Allocator>& operator+=(T other)
    {
        pushBack(other);
        return *this;
    }

    void append(const BasicString<T, Allocator>& other)
    {
        append(other.data(), traits_type::length(other.data()));
    }

    void append(const T* src)
    {
        append(src, traits_type::length(src));
    }

    void append(const T* src, size_t len);

    void insert(size_t pos, const T val);

    template <class _Iterator, class _InputIterator>
    void insert(_Iterator pos, _InputIterator first, _InputIterator last)
    {
        replace(pos, pos, first, last);
    }

    template <class _Iterator>
    _Iterator erase(_Iterator first, _Iterator last);

    void erase(size_t pos)
    {
        erase(pos, pos + 1);
    }

    void erase(size_t start, size_t end);
    template <class _Iterator>
    _Iterator erase(_Iterator pos)
    {
        return erase(pos, pos + 1);
    }

    template <class _Iterator, class _InputIterator>
    BasicString<T, Allocator>& replace(_Iterator startToErase,
                                       _Iterator endToErase,
                                       _InputIterator startToInsert,
                                       _InputIterator endToInsert)
    {
        typedef typename std::__is_integer<_InputIterator>::__type _Integral;
        return replace_dispatch(startToErase, endToErase, startToInsert,
                                endToInsert, _Integral());
    }

    BasicString<T, Allocator>& replace(size_t pos, size_t len,
                                       const BasicString<T, Allocator>& dst);

    BasicString<T, Allocator>& replace(size_t pos, size_t len, size_t n, T c);

    BasicString<T, Allocator> substr(size_t pos = 0, size_t len = 0) const;

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
        return m_capacity;
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

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
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
        makeEmpty();
    }

    T* data() const
    {
        STARFISH_ASSERT(m_buffer != nullptr);
        return m_buffer;
    }

    void swap(BasicString<T, Allocator>& other)
    {
        T* buffer = m_buffer;
        m_buffer = other.m_buffer;
        other.m_buffer = buffer;
    }

    void resize(size_t newSize)
    {
        reserve(newSize);
        memset(m_buffer + m_size, 0, (newSize - m_size) + sizeof(T));
        setLen(newSize);
    }

    void reserve(size_t newSize);

    int compare(const BasicString<T, Allocator>& other) const;
    int compare(const T* s) const;

    T* takeBuffer()
    {
        T* buffer = m_buffer;
        AllocInfo<T> allocInfo = allocate(1);
        m_buffer = allocInfo.m_buffer;
        m_buffer[0] = 0;
        m_size = 0;
        m_capacity = 1;

        return buffer;
    }

protected:
    void makeEmpty()
    {
        AllocInfo<T> allocInfo = allocate(1);
        resetBuffer(allocInfo);
        setLen(0);
    }

    constexpr size_t toCapacity(size_t size) const
    {
        if (size == 0) {
            return 1;
        }
        size_t base = log2l(size);
        size_t capacity = 1 << (base + 1);
        return capacity;
    }

    AllocInfo<T> allocate(size_t size) const
    {
        size_t capacity = toCapacity(size);
        T* ret = Allocator().allocate(capacity);
        return AllocInfo<T>(ret, capacity);
    }

    void resetBuffer(AllocInfo<T>& info)
    {
        deallocate();
        m_buffer = info.m_buffer;
        m_capacity = info.m_capacity;
    }

    template <typename _InputIterator>
    void construct(_InputIterator start, _InputIterator end)
    {
        typedef typename std::__is_integer<_InputIterator>::__type _Integral;
        _construct(start, end, _Integral());
    }

    template <class _Integer>
    void _construct(_Integer start, _Integer end, std::__true_type)
    {
        _construct(static_cast<size_t>(start), static_cast<T>(end));
    }

    void _construct(size_t size, T c)
    {
        AllocInfo<T> allocInfo = allocate(size + 1);
        memset(allocInfo.m_buffer, c, sizeof(T) * size);
        resetBuffer(allocInfo);
        setLen(size);
    }

    template <class _InIterator>
    void _construct(_InIterator start, _InIterator end, std::__false_type)
    {
        typedef
            typename std::iterator_traits<_InIterator>::iterator_category _Tag;
        return _construct(start, end, _Tag());
    }

    template <class _InIterator>
    void _construct(_InIterator start, _InIterator end,
                    std::forward_iterator_tag)
    {
        STARFISH_ASSERT(!(__gnu_cxx::__is_null_pointer(start) && start != end));
        const size_t distance = static_cast<size_t>(std::distance(start, end));
        AllocInfo<T> allocInfo = allocate(distance + 1);
        resetBuffer(allocInfo);
        size_t len = 0;
        for (; len < distance; len++) {
            m_buffer[len] = *(start + len);
        }
        setLen(len);
    }

    template <class _InIterator>
    void _construct(_InIterator start, _InIterator end, std::input_iterator_tag)
    {
        T buf[128];
        size_t len = 0;
        while (start != end && len < sizeof(buf) / sizeof(T)) {
            buf[len++] = *start;
            ++start;
        }

        AllocInfo<T> allocInfo = allocate(len + 1);
        memcpy(allocInfo.m_buffer, buf, len * sizeof(T));
        resetBuffer(allocInfo);

        while (start != end) {
            size_t newLen = len + 1;
            if (m_capacity <= newLen) {
                {
                    AllocInfo<T> allocInfo = allocate(newLen + 1);
                    memcpy(allocInfo.m_buffer, m_buffer, len * sizeof(T));
                    resetBuffer(allocInfo);
                    setLen(newLen);
                }
                m_buffer[len++] = *start;
                ++start;
            }
        }
    }

    void construct(const T* src, size_t size)
    {
        STARFISH_ASSERT(src);
        AllocInfo<T> allocInfo = allocate(size + 1);
        memcpy(allocInfo.m_buffer, src, sizeof(T) * size);
        resetBuffer(allocInfo);
        setLen(size);
    }

    void construct(const BasicString<T, Allocator>& other)
    {
        construct(other.m_buffer, other.m_size);
    }

    void setLen(size_t newLen)
    {
        m_buffer[newLen] = 0;
        m_size = newLen;
    }

    void mutate(size_t pos, size_t sizeToErase, size_t sizeToInsert);

    void pushBack(T val);

    std::tuple<T*, size_t> _substr(size_t pos, size_t len) const;

    template <class _Iterator, class _Integer>
    BasicString<T, Allocator>& replace_dispatch(_Iterator startToErase,
                                                _Iterator endToErase,
                                                _Integer n, _Integer val,
                                                std::__true_type)
    {
        return replace(startToErase - begin(), endToErase - startToErase, n,
                       val);
    }

    template <class _Iterator, class _InputIterator>
    BasicString<T, Allocator>& replace_dispatch(_Iterator startToErase,
                                                _Iterator endToErase,
                                                _InputIterator startToInsert,
                                                _InputIterator endToInsert,
                                                std::__false_type);

    // Important! `m_size` update should follow `deallocate()`
    void deallocate()
    {
        if (m_buffer) {
            Allocator().deallocate(m_buffer, m_capacity);
        }
        m_buffer = nullptr;
        m_size = 0;
        m_capacity = 0;
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
    size_t m_capacity;
};
}

#endif

#include "core/util/BasicString.hpp"
