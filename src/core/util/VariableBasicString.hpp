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

#ifndef __StarFishVariableBasicStringImpl__
#define __StarFishVariableBasicStringImpl__

namespace StarFish {

template <typename T, typename Allocator>
void VariableBasicString<T, Allocator>::pushBack(T val)
{
    size_t size = this->m_size;
    size_t newLen = size + 1;
    if (capacity() > newLen) {
        T* data = this->data();
        data[newLen - 1] = val;
        data[newLen] = 0;
        this->m_size = newLen;
    } else {
        extend(newLen);
        pushBack(val);
    }
}

template <typename T, typename Allocator>
void VariableBasicString<T, Allocator>::append(const T* src, size_t len)
{
    size_t size = this->m_size;
    size_t newLen = size + len;
    if (capacity() > newLen) {
        T* data = this->data();
        memcpy(data + size, src, len * sizeof(T));
        data[newLen] = 0;
        this->m_size = newLen;
    } else {
        extend(newLen);
        append(src, len);
    }
}

template <typename T, typename Allocator>
void VariableBasicString<T, Allocator>::insert(size_t pos, const T val)
{
    size_t size = this->m_size;
    ASSERT(pos < size);
    size_t newLen = size + 1;
    if (capacity() > newLen) {
        T* data = this->data();
        memmove(data + pos + 1, data + pos, size - pos);
        data[pos] = val;
        data[newLen] = 0;
        this->m_size = newLen;
    } else {
        extend(newLen);
        insert(pos, val);
    }
}

template <typename T, typename Allocator>
void VariableBasicString<T, Allocator>::erase(size_t start, size_t end)
{
    size_t size = this->m_size;
    ASSERT(start < end);
    ASSERT(start >= 0);
    ASSERT(end <= size);

    size_t howMuch = end - start;
    size_t newLen = size - howMuch;

    T* data = this->data();
    memmove(data + start, data + start + howMuch, newLen - start);
    data[newLen] = 0;
    this->m_size = newLen;
}

template <typename T, typename Allocator>
VariableBasicString<T, Allocator>& VariableBasicString<T, Allocator>::replace(size_t start, size_t len, BasicString<T, Allocator>& dst)
{
    size_t size = this->m_size;
    size_t osize = dst.size();
    STARFISH_ASSERT(start < size);
    size_t howMuch = std::min(size - start - 1, len);
    size_t newLen = size - howMuch + osize;

    if (capacity() > newLen) {
        T* data = this->data();
        memmove(data + start + osize, data + start + howMuch, size - start - howMuch);
        memcpy(data + start, dst.data(), sizeof(T) * osize);
        data[newLen] = 0;
        this->m_size = newLen;

        return *this;
    } else {
        extend(newLen);
        return replace(start, len, dst);
    }
}

template <typename T, typename Allocator>
VariableBasicString<T, Allocator> VariableBasicString<T, Allocator>::substr(size_t pos, size_t len) const
{
    std::tuple<T*, size_t> data = this->_substr(pos, len);
    T* buffer = std::get<0>(data);
    size_t newLen = std::get<1>(data);

    return VariableBasicString(buffer, newLen);
}
}
#endif
