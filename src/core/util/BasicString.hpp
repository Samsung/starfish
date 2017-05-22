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

#ifndef __StarFishBasicStringImpl__
#define __StarFishBasicStringImpl__

namespace StarFish {

template <typename T, typename Allocator>
T* BasicString<T, Allocator>::allocate(size_t size) const
{
    T* ret = Allocator().allocate(size);
    return ret;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::pushBack(T val)
{
    size_t newLen = m_size + 1;
    T* newBuffer = allocate(newLen + 1);
    memcpy(newBuffer, m_buffer, m_size * sizeof(T));
    newBuffer[newLen - 1] = val;
    newBuffer[newLen] = 0;
    deallocate();
    m_buffer = newBuffer;
    m_size = newLen;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::append(const T* src, size_t len)
{
    size_t newLen = m_size + len;
    T* newBuffer = allocate(newLen + 1);
    memcpy(newBuffer, m_buffer, m_size * sizeof(T));
    memcpy(newBuffer + m_size, src, len * sizeof(T));
    newBuffer[newLen] = 0;
    deallocate();
    m_buffer = newBuffer;
    m_size = newLen;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::insert(size_t pos, const T val)
{
    STARFISH_ASSERT(pos < m_size);
    size_t newLen = m_size + 1;
    T* newBuffer = allocate(newLen + 1);
    memcpy(newBuffer, m_buffer, pos * sizeof(T));
    newBuffer[pos] = val;
    memcpy(newBuffer + pos + 1, m_buffer + pos, (m_size - pos) * sizeof(T));
    newBuffer[newLen] = 0;
    deallocate();
    m_buffer = newBuffer;
    m_size = newLen;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::erase(size_t start, size_t end)
{
    STARFISH_ASSERT(start < end);
    STARFISH_ASSERT(start >= 0);
    STARFISH_ASSERT(end <= m_size);

    size_t howMuch = end - start;
    size_t newLen = m_size - howMuch;

    if (newLen == 0) {
        deallocate();
    } else {
        T* newBuffer = allocate(newLen + 1);
        memcpy(newBuffer, m_buffer, start * sizeof(T));
        memcpy(newBuffer + start, m_buffer + howMuch, (m_size - start - howMuch) * sizeof(T));
        newBuffer[newLen] = 0;

        deallocate();
        m_buffer = newBuffer;
        m_size = newLen;
    }
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_t start, size_t len, BasicString<T, Allocator>& dst)
{
    STARFISH_ASSERT(start < m_size);
    size_t howMuch = std::min(m_size - start - 1, len);
    size_t newLen = m_size - howMuch + dst.m_size;

    T* newBuffer = allocate(newLen + 1);
    const T* data = this->data();

    memcpy(newBuffer, data, sizeof(T) * start);
    memcpy(newBuffer + start, dst.data(), sizeof(T) * dst.m_size);
    memcpy(newBuffer + start + dst.m_size, data + start + howMuch, sizeof(T) * (m_size - howMuch - start));
    newBuffer[newLen] = 0;

    deallocate();
    m_buffer = newBuffer;
    m_size = newLen;

    return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> BasicString<T, Allocator>::substr(size_t pos, size_t len) const
{
    std::tuple<T*, size_t> data = _substr(pos, len);
    T* buffer = std::get<0>(data);
    size_t newLen = std::get<1>(data);

    return BasicString(buffer, newLen);
}

template <typename T, typename Allocator>
std::tuple<T*, size_t> BasicString<T, Allocator>::_substr(size_t pos, size_t len) const
{
    STARFISH_ASSERT(m_size - len >= pos);
    size_t newLen = std::min(len, m_size - pos);
    T* buffer = allocate(newLen + 1);
    memcpy(buffer, data() + pos, sizeof(T) * newLen);
    buffer[newLen] = 0;

    return std::make_tuple(buffer, newLen);
}

template <typename T, typename Allocator>
size_t BasicString<T, Allocator>::find(T c, size_t pos) const
{
    size_t ret = SIZE_MAX;
    const size_t size = this->size();
    if (pos < size)
    {
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
}

#endif
