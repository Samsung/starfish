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

#ifndef __StarFishVariableBasicString__
#define __StarFishVariableBasicString__

#include "StarFishConfig.h"
#include "core/util/BasicString.h"

namespace StarFish {

template <typename T, typename Allocator>
class VariableBasicString : public BasicString<T, Allocator> {
public:
    VariableBasicString()
        : BasicString<T, Allocator>()
        , m_capacity(1)
    {
    }

    VariableBasicString(const T* src)
        : BasicString<T, Allocator>(src)
        , m_capacity(this->m_size + 1)
    {
    }

    VariableBasicString(const T* src, size_t len)
        : BasicString<T, Allocator>(src, len)
        , m_capacity(this->m_size + 1)
    {
    }

    VariableBasicString(VariableBasicString<T, Allocator>&& other)
        : BasicString<T, Allocator>(other)
        , m_capacity(this->m_size + 1)
    {
    }

    VariableBasicString(const VariableBasicString<T, Allocator>& other)
        : BasicString<T, Allocator>(other)
        , m_capacity(this->m_size + 1)
    {
    }

    const VariableBasicString<T, Allocator>& operator=(
        const VariableBasicString<T, Allocator>& other)
    {
        if (other.m_size > 0) {
            size_t capacity = other.m_size + 1;
            this->copy(other, capacity);
            m_capacity = capacity;
        } else {
            this->clear();
        }
        return *this;
    }

    ~VariableBasicString()
    {
        deallocate();
    }

    void resize(size_t newSize)
    {
        BasicString<T, Allocator>::resize(newSize);
        m_capacity = newSize + 1;
    }

    void reserve(size_t newSize)
    {
        BasicString<T, Allocator>::reserve(newSize);
        m_capacity = newSize + 1;
    }

    void pushBack(T val);
    void append(const T* src, size_t len);
    void append(VariableBasicString<T, Allocator> other)
    {
        BasicString<T, Allocator>::append(other);
    }
    void insert(size_t pos, const T val);
    void erase(size_t start, size_t end);
    VariableBasicString<T, Allocator>& replace(size_t start, size_t len,
                                               BasicString<T, Allocator>& dst);
    VariableBasicString<T, Allocator> substr(size_t pos, size_t len) const;

    size_t capacity() const
    {
        return m_capacity;
    }

    const VariableBasicString<T, Allocator> operator+(
        const VariableBasicString<T, Allocator>& other)
    {
        VariableBasicString ret(*this);
        ret.append(other);
        return ret;
    }

    const VariableBasicString<T, Allocator>& operator+=(
        const VariableBasicString<T, Allocator>& other)
    {
        append(other);
        return *this;
    }

    const VariableBasicString<T, Allocator>& operator+=(T other)
    {
        pushBack(other);
        return *this;
    }

protected:
    constexpr size_t toCapacity(size_t size) const
    {
        if (size == 0) {
            return 1;
        }
        size_t base = log2l(size + 1);
        size_t capacity = 1 << (base + 1);
        return capacity;
    }

    void makeEmpty()
    {
        BasicString<T, Allocator>::makeEmpty();
        m_capacity = 1;
    }

    void deallocate()
    {
        BasicString<T, Allocator>::deallocate();
        m_capacity = 0;
    }

    void extend(size_t newSize)
    {
        size_t capacity = toCapacity(newSize);
        size_t size = this->m_size;
        T* newBuffer = this->allocate(capacity);
        memcpy(newBuffer, this->data(), sizeof(T) * size);
        newBuffer[size] = 0;
        deallocate();
        m_capacity = capacity;
        this->m_size = size;
        this->m_buffer = newBuffer;
    }

private:
    mutable size_t m_capacity;
};
}
#endif

#include "core/util/VariableBasicString.hpp"
