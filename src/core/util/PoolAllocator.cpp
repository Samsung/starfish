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

#include "StarfishConfig.h"
#include "PoolAllocator.h"

namespace Starfish {

PoolAllocator::PoolAllocator()
{
    m_poolMemory = static_cast<char*>(malloc(poolSize()));
    m_poolEnd = m_poolMemory + poolSize();
}

PoolAllocator::~PoolAllocator()
{
    reset();

    STARFISH_ASSERT(m_pools.size() == 0);
    STARFISH_ASSERT(m_poolMemory == currentPool());

    free(currentPool());
}

void PoolAllocator::reset()
{
    STARFISH_ASSERT(m_poolMemory != nullptr && m_poolEnd != nullptr);
    STARFISH_ASSERT(static_cast<size_t>(m_poolEnd - m_poolMemory) >= 0);

    if (m_pools.size()) {
        m_poolMemory = static_cast<char*>(m_pools[0]);

        free(m_poolEnd - poolSize());
        for (size_t i = 1; i < m_pools.size(); i++) {
            free(m_pools[i]);
        }
        m_pools.clear();
        m_poolEnd = m_poolMemory + poolSize();
    } else {
        m_poolMemory = static_cast<char*>(currentPool());
    }
}

void* PoolAllocator::allocate(size_t size)
{
    STARFISH_ASSERT(size > 0);
    STARFISH_ASSERT(size <= poolSize());
    size_t alignedSize = alignSize(size);
    STARFISH_ASSERT(alignedSize <= poolSize());
    if (UNLIKELY(static_cast<size_t>(m_poolEnd - m_poolMemory) < alignedSize)) {
        allocatePool();
    }
    void* block = m_poolMemory;
    m_poolMemory += alignedSize;
    return block;
}

void PoolAllocator::allocatePool()
{
    STARFISH_ASSERT(m_poolMemory != nullptr && m_poolEnd != nullptr);
    STARFISH_ASSERT(static_cast<size_t>(m_poolEnd - m_poolMemory) >= 0);

    m_pools.push_back(currentPool());

    char* pool = static_cast<char*>(malloc(poolSize()));
    m_poolMemory = pool;
    m_poolEnd = pool + poolSize();
}
}
