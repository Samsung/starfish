/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishVectorWithInlineStorage__
#define __StarfishVectorWithInlineStorage__

namespace Starfish {

// Vector for special purpose
// It has InlineStorage, so push_back operation is fast with InlineStorage
// It is good for gathering small amount datas repeatedly(I think)
template <unsigned int InlineStorageSize, typename T,
          typename ExternalStoreageAllocator>
class VectorWithInlineStorage : public gc {
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    VectorWithInlineStorage()
    {
        m_size = 0;
    }

    VectorWithInlineStorage(
        const VectorWithInlineStorage<InlineStorageSize, T,
                                      ExternalStoreageAllocator>& src)
    {
        operator=(src);
    }

    VectorWithInlineStorage(
        VectorWithInlineStorage<InlineStorageSize, T,
                                ExternalStoreageAllocator>&& src)
    {
        operator=(std::move(src));
    }

    VectorWithInlineStorage<InlineStorageSize, T, ExternalStoreageAllocator>&
    operator=(VectorWithInlineStorage<InlineStorageSize, T,
                                      ExternalStoreageAllocator>&& src)
    {
        m_size = src.m_size;

        if (LIKELY(m_size <= InlineStorageSize)) {
            for (size_t i = 0; i < m_size; i++) {
                m_inlineStorage[i] = std::move(src.m_inlineStorage[i]);
            }
        } else {
            m_externalStorage = std::move(src.m_externalStorage);
        }
        src.m_size = 0;
        return *this;
    }

    VectorWithInlineStorage<InlineStorageSize, T, ExternalStoreageAllocator>&
    operator=(const VectorWithInlineStorage<InlineStorageSize, T,
                                            ExternalStoreageAllocator>& src)
    {
        m_size = src.m_size;
        if (LIKELY(m_size <= InlineStorageSize)) {
            for (size_t i = 0; i < m_size; i++) {
                m_inlineStorage[i] = src.m_inlineStorage[i];
            }
        } else {
            m_externalStorage = src.m_externalStorage;
        }
        return *this;
    }

    void push_back(const T& decl)
    {
        if (LIKELY(m_size < InlineStorageSize)) {
            m_inlineStorage[m_size] = decl;
        } else if (m_size == InlineStorageSize) {
            m_externalStorage.assign(m_inlineStorage, m_inlineStorage + m_size);
            m_externalStorage.push_back(decl);
        } else {
            m_externalStorage.push_back(decl);
        }
        m_size++;
    }

    void push_back(T&& decl)
    {
        if (LIKELY(m_size < InlineStorageSize)) {
            m_inlineStorage[m_size] = std::move(decl);
        } else if (m_size == InlineStorageSize) {
            m_externalStorage.assign(m_inlineStorage, m_inlineStorage + m_size);
            m_externalStorage.push_back(std::move(decl));
        } else {
            m_externalStorage.push_back(std::move(decl));
        }
        m_size++;
    }

    void pop_back()
    {
        m_size--;
        if (m_size == InlineStorageSize) {
            for (size_t i = 0; i < m_size; i++) {
                m_inlineStorage[i] = m_externalStorage[i];
            }
            m_externalStorage.clear();
        } else if (m_size > InlineStorageSize) {
            m_externalStorage.pop_back();
        }
    }

    T& operator[](const size_t& idx)
    {
        if (LIKELY(m_size <= InlineStorageSize)) {
            return m_inlineStorage[idx];
        } else {
            return m_externalStorage[idx];
        }
    }

    const T& operator[](const size_t& idx) const
    {
        if (LIKELY(m_size <= InlineStorageSize)) {
            return m_inlineStorage[idx];
        } else {
            return m_externalStorage[idx];
        }
    }

    T* data()
    {
        if (LIKELY(m_size <= InlineStorageSize)) {
            return m_inlineStorage;
        } else {
            return m_externalStorage.data();
        }
    }

    size_t size() const
    {
        return m_size;
    }

    size_t length() const
    {
        return m_size;
    }

    void clear()
    {
        m_size = 0;
        m_externalStorage.clear();
    }

    iterator begin()
    {
        return iterator(data());
    }

    const_iterator cbegin()
    {
        return const_iterator(data());
    }

    T& front()
    {
        return *begin();
    }

    iterator end()
    {
        return iterator(data() + m_size);
    }

    const_iterator cend()
    {
        return const_iterator(data() + m_size);
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

protected:
    size_t m_size;
    T m_inlineStorage[InlineStorageSize];
    std::vector<T, ExternalStoreageAllocator> m_externalStorage;
};
} // namespace Starfish

#endif
