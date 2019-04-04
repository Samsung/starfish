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

template <typename T>
class Id {
    friend struct IdHash;

public:
    Id() = default;

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
        return m_id;
    }

    static Id<T> generate()
    {
        static size_t currentId;

        if (UNLIKELY(currentId == std::numeric_limits<size_t>::max())) {
            STARFISH_LOG_WARN(
                "the Id count is reset since it reaches the end of its value.");
            currentId = 0;
        }
        return Id<T>(++currentId);
    }

    std::string toString() const
    {
        return std::to_string(m_id);
    }

    bool isValid()
    {
        return m_id != 0;
    }

private:
    explicit Id(size_t id)
        : m_id(id)
    {
    }

    size_t m_id{ 0 };
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
