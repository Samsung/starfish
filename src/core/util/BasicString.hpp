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

#ifndef __StarFishBasicStringImpl__
#define __StarFishBasicStringImpl__

namespace StarFish {

template <typename T, typename Allocator>
void BasicString<T, Allocator>::pushBack(T val)
{
    size_t newLen = m_size + 1;
    reserve(newLen);
    m_buffer[newLen - 1] = val;
    setLen(newLen);
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::append(const T* src, size_t len)
{
    if (len == 0) {
        return;
    }
    STARFISH_ASSERT(src);

    size_t newLen = m_size + len;
    reserve(newLen);
    memcpy(m_buffer + m_size, src, len * sizeof(T));
    setLen(newLen);
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::insert(size_t pos, const T val)
{
    STARFISH_ASSERT(pos < m_size);
    mutate(pos, 0, 1);
    m_buffer[pos] = val;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::erase(size_t start, size_t end)
{
    STARFISH_ASSERT(start < end);
    STARFISH_ASSERT(start >= 0);
    STARFISH_ASSERT(end <= m_size);

    size_t howMuch = end - start;
    size_t newLen = m_size - howMuch;

    if (howMuch == 0) {
        return;
    } else if (newLen == 0) {
        clear();
    } else {
        mutate(start, howMuch, 0);
    }
}

template <typename T, typename Allocator>
template <class _Iterator>
_Iterator BasicString<T, Allocator>::erase(_Iterator first, _Iterator last)
{
    size_t start = first - _Iterator(data());
    size_t end = last - _Iterator(data());

    erase(start, end);

    return _Iterator(this->m_buffer + start);
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(
    size_t pos, size_t len, const BasicString<T, Allocator>& dst)
{
    mutate(pos, len, dst.m_size);
    if (dst.m_size > 0) {
        memcpy(m_buffer + pos, dst.m_buffer, sizeof(T) * dst.m_size);
    }

    return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_t pos,
                                                              size_t len,
                                                              size_t n, T c)
{
    mutate(pos, len, n);
    if (n > 0) {
        memset(m_buffer + pos, c, n * sizeof(T));
    }

    return *this;
}

template <typename T, typename Allocator>
template <class _Iterator, class _InputIterator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace_dispatch(
    _Iterator startToErase, _Iterator endToErase, _InputIterator startToInsert,
    _InputIterator endToInsert, std::false_type)
{
    const BasicString s(startToInsert, endToInsert);
    const size_t sizeToErase = endToErase - startToErase;
    const size_t pos = startToErase - _Iterator(this->data());
    return this->replace(pos, sizeToErase, s);
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::reserve(size_t newSize)
{
    if (newSize == 0) {
        clear();
        return;
    }

    size_t len = std::min(m_size, newSize);
    if (this->m_capacity <= newSize) {
        AllocInfo<T> allocInfo = allocate(newSize + 1);

        if (len > 0) {
            memcpy(allocInfo.m_buffer, m_buffer, len * sizeof(T));
        }

        resetBuffer(allocInfo);
    }

    setLen(len);
}

template <typename T, typename Allocator>
int BasicString<T, Allocator>::compare(
    const BasicString<T, Allocator>& other) const
{
    const size_t osize = other.size();
    const size_t len = std::min(m_size, osize);

    int r = traits_type::compare(data(), other.data(), len);
    if (!r) {
        r = _compare(m_size, osize);
    }
    return r;
}

template <typename T, typename Allocator>
int BasicString<T, Allocator>::compare(const T* other) const
{
    const size_t osize = traits_type::length(other);
    const size_t len = std::min(m_size, osize);
    int r = traits_type::compare(data(), other, len);
    if (!r) {
        r = _compare(m_size, osize);
    }
    return r;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::mutate(size_t pos, size_t sizeToErase,
                                       size_t sizeToInsert)
{
    STARFISH_ASSERT(pos < m_size);
    STARFISH_ASSERT(sizeToErase + pos <= m_size);
    size_t newLen = m_size - sizeToErase + sizeToInsert;
    size_t howMuch = m_size - pos - sizeToErase;

    if (this->m_capacity > newLen) {
        if (howMuch > 0) {
            memmove(m_buffer + pos + sizeToInsert, m_buffer + pos + sizeToErase,
                    howMuch * sizeof(T));
        }
    } else {
        AllocInfo<T> allocInfo = allocate(newLen + 1);

        if (pos > 0) {
            memcpy(allocInfo.m_buffer, m_buffer, sizeof(T) * pos);
        }

        if (howMuch > 0) {
            memcpy(allocInfo.m_buffer + pos + sizeToInsert,
                   m_buffer + pos + sizeToErase, howMuch * sizeof(T));
        }

        resetBuffer(allocInfo);
    }
    setLen(newLen);
}

template <typename T, typename Allocator>
BasicString<T, Allocator> BasicString<T, Allocator>::substr(size_t pos,
                                                            size_t len) const
{
    std::tuple<T*, size_t> data = _substr(pos, len);
    T* buffer = std::get<0>(data);
    size_t newLen = std::get<1>(data);

    return BasicString(buffer, newLen);
}

template <typename T, typename Allocator>
std::tuple<T*, size_t> BasicString<T, Allocator>::_substr(size_t pos,
                                                          size_t len) const
{
    STARFISH_ASSERT(m_size - len >= pos);
    size_t newLen = std::min(len, m_size - pos);
    AllocInfo<T> allocInfo = allocate(newLen + 1);
    if (newLen > 0) {
        memcpy(allocInfo.m_buffer, data() + pos, sizeof(T) * newLen);
    }
    allocInfo.m_buffer[newLen] = 0;

    return std::make_tuple(allocInfo.m_buffer, newLen);
}

template <typename T, typename Allocator>
size_t BasicString<T, Allocator>::find(T c, size_t pos) const
{
    size_t ret = SIZE_MAX;
    const size_t size = this->size();
    if (pos < size) {
        const T* data = this->data();
        const size_type n = size - pos;
        const T* p = traits_type::find(data + pos, n, c);
        if (p) {
            ret = p - data;
        }
    }
    return ret;
}

template <typename T, typename Allocator>
size_t BasicString<T, Allocator>::find(const T* src, size_t pos, size_t n) const
{
    const T* data = this->data();

    if (n == 0) {
        return pos <= m_size ? pos : SIZE_MAX;
    }

    if (n <= m_size) {
        for (; pos <= m_size - n; ++pos) {
            if (traits_type::eq(data[pos], src[0]) &&
                traits_type::compare(data + pos + 1, src + 1, n - 1) == 0) {
                return pos;
            }
        }
    }
    return SIZE_MAX;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(const BasicString<T, Allocator>& lhs,
                                    const BasicString<T, Allocator>& rhs)
{
    BasicString<T, Allocator> ret(lhs);
    ret.append(rhs);
    return ret;
}

template <typename T, typename Allocator>
inline bool operator==(const BasicString<T, Allocator>& lhs,
                       const BasicString<T, Allocator>& rhs)
{
    return lhs.compare(rhs) == 0;
}

template <typename T, typename Allocator>
inline bool operator==(const BasicString<T, Allocator>& lhs, const T* rhs)
{
    return lhs.compare(rhs) == 0;
}

template <typename T, typename Allocator>
inline bool operator==(const T* lhs, const BasicString<T, Allocator>& rhs)
{
    return rhs.compare(lhs) == 0;
}

template <typename T, typename Allocator>
inline bool operator!=(const BasicString<T, Allocator>& lhs,
                       const BasicString<T, Allocator>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename Allocator>
inline bool operator!=(const T* lhs, const BasicString<T, Allocator>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename Allocator>
inline bool operator!=(const BasicString<T, Allocator>& lhs, const T* rhs)
{
    return !(lhs == rhs);
}
}

#endif
