/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

    size_t size() const
    {
        return m_size;
    }

    void clear()
    {
        m_size = 0;
        m_externalStorage.clear();
    }

    template <typename Func>
    void sortVector(Func matchingRule)
    {
        if (m_size <= InlineStorageSize) {
            std::stable_sort(m_inlineStorage, m_inlineStorage + m_size,
                             matchingRule);
        } else {
            std::stable_sort(m_externalStorage.begin(), m_externalStorage.end(),
                             matchingRule);
        }
    }

protected:
    size_t m_size;
    T m_inlineStorage[InlineStorageSize];
    std::vector<T, ExternalStoreageAllocator> m_externalStorage;
};
}

#endif
