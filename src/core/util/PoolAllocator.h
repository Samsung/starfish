/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPoolAllocator__
#define __StarfishPoolAllocator__

namespace Starfish {

class PoolAllocator {
public:
    typedef intptr_t PoolAllocAlignment;

    PoolAllocator();
    ~PoolAllocator();

    void reset();
    void* allocate(size_t size);

    bool isInitialized()
    {
        STARFISH_ASSERT(m_pools.size() == 0);
        return (m_poolMemory == currentPool());
    }

private:
    inline size_t poolSize() const
    {
        const size_t poolSizeMap[] = { 1024 * 4, 1024 * 16, 1024 * 128 };
        if (m_pools.size() >= (sizeof(poolSizeMap) / sizeof(size_t))) {
            return poolSizeMap[(sizeof(poolSizeMap) / sizeof(size_t)) - 1];
        }
        return poolSizeMap[m_pools.size()];
    }

    size_t alignSize(size_t size)
    {
        return (size + sizeof(PoolAllocAlignment) - 1) &
               ~(sizeof(PoolAllocAlignment) - 1);
    }
    void allocatePool();
    void* currentPool()
    {
        STARFISH_ASSERT(m_poolMemory != nullptr && m_poolEnd != nullptr);
        STARFISH_ASSERT(static_cast<size_t>(m_poolEnd - m_poolMemory) >= 0);

        return m_poolEnd - poolSize();
    }

    char* m_poolMemory;
    char* m_poolEnd;

    std::vector<void*> m_pools;
};
} // namespace Starfish
#endif
