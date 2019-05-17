/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishId__
#define __StarfishId__

namespace Starfish {

enum class CreatedIdType { UNIQUE, SHARED };

#define ID_INITIAL_VALUE 0

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
        static unsigned currentId;

        if (UNLIKELY(currentId == std::numeric_limits<unsigned>::max())) {
            STARFISH_LOG_WARN(
                "the Id count is reset since it reaches the end of its value.");
            currentId = ID_INITIAL_VALUE;
        }
        return Id<T>(++currentId);
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
    explicit Id(unsigned id)
        : m_id(id)
        , m_createdType(CreatedIdType::UNIQUE)
    {
    }

    unsigned m_id;
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
