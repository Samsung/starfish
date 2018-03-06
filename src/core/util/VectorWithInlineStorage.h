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

#ifndef __StarFishVectorWithInlineStorage__
#define __StarFishVectorWithInlineStorage__

namespace StarFish {

// Vector for special purpose
// It has InlineStorage, so push_back operation is fast with InlineStorage
// It is good for gathering small amount datas repeatedly(I think)
template <unsigned int InlineStorageSize, typename T,
          typename ExternalStoreageAllocator>
class VectorWithInlineStorage : public gc {
public:
    VectorWithInlineStorage()
    {
        m_size = 0;
    }

    VectorWithInlineStorage<InlineStorageSize, T, ExternalStoreageAllocator>&
    operator=(const VectorWithInlineStorage<
              InlineStorageSize, T, ExternalStoreageAllocator>&& src) = delete;
    VectorWithInlineStorage<InlineStorageSize, T, ExternalStoreageAllocator>&
    operator=(const VectorWithInlineStorage<
              InlineStorageSize, T, ExternalStoreageAllocator>& src) = delete;

    void push_back(const T& decl)
    {
        if (m_size < InlineStorageSize) {
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
        if (m_size < InlineStorageSize) {
            m_inlineStorage[m_size] = std::move(decl);
        } else if (m_size == InlineStorageSize) {
            m_externalStorage.assign(m_inlineStorage, m_inlineStorage + m_size);
            m_externalStorage.push_back(std::move(decl));
        } else {
            m_externalStorage.push_back(std::move(decl));
        }
        m_size++;
    }

    T& operator[](const size_t& idx)
    {
        if (m_size <= InlineStorageSize) {
            return m_inlineStorage[idx];
        } else {
            return m_externalStorage[idx];
        }
    }

    const T& operator[](const size_t& idx) const
    {
        if (m_size <= InlineStorageSize) {
            return m_inlineStorage[idx];
        } else {
            return m_externalStorage[idx];
        }
    }

    T* data()
    {
        if (m_size <= InlineStorageSize) {
            return m_inlineStorage;
        } else {
            return m_externalStorage.data();
        }
    }

    size_t size() const
    {
        return m_size;
    }

    void clear()
    {
        m_size = 0;
        m_externalStorage.clear();
    }

protected:
    size_t m_size;
    T m_inlineStorage[InlineStorageSize];
    std::vector<T, ExternalStoreageAllocator> m_externalStorage;
};
}

#endif
