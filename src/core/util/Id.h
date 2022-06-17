/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishId__
#define __StarfishId__

#include <functional>

namespace Starfish {

enum class CreatedIdType { UNIQUE, SHARED };

#define ID_INITIAL_VALUE 0

class IDGenerator {
    using StrategyFunc = std::function<uint32_t()>;

public:
    static uint32_t sequence();
    static uint32_t random();

    IDGenerator(StrategyFunc strategy = random)
        : m_strategyFn(strategy)
    {
    }

    uint32_t getNewId()
    {
        return m_strategyFn();
    }

private:
    StrategyFunc m_strategyFn;
};

template <typename T>
class Id {
    friend struct IdHash;
    friend class Archiver;

public:
    Id()
        : m_id(ID_INITIAL_VALUE)
        , m_createdType(CreatedIdType::SHARED)
    {
    }

    bool operator==(const Id& rhs) const
    {
        return m_id == rhs.m_id;
    }

    bool operator!=(const Id& rhs) const
    {
        return m_id != rhs.m_id;
    }

    explicit operator bool() const
    {
        return (m_id != ID_INITIAL_VALUE);
    }

    static Id<T> generate()
    {
        static IDGenerator generator;

        return Id<T>(generator.getNewId());
    }

    std::string toString() const
    {
        return std::to_string(m_id);
    }

    bool isValid()
    {
        return (m_id != ID_INITIAL_VALUE);
    }

    bool isUnique()
    {
        return ((m_id != ID_INITIAL_VALUE) &&
                (m_createdType == CreatedIdType::UNIQUE));
    }

private:
    explicit Id(uint32_t id)
        : m_id(id)
        , m_createdType(CreatedIdType::UNIQUE)
    {
    }

    uint32_t m_id;
    CreatedIdType m_createdType;
};

struct IdHash {
    template <typename T>
    std::size_t operator()(const Id<T>& id) const
    {
        return id.m_id;
    }
};

} // namespace Starfish

#endif
