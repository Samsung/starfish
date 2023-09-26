/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishLengthDirectionAwereData__
#define __StarfishLengthDirectionAwereData__

namespace Starfish {

class ComputedStyle;

class LengthInlineDirectionAwereData : public gc {
public:
    LengthInlineDirectionAwereData()
        : m_length(Length(Length::Fixed, 0))
        , m_isCorrespondingLeftSet(false)
        , m_isCorrespondingRightSet(false)

    {
    }

    LengthInlineDirectionAwereData(Length length)
        : m_length(length)
        , m_isCorrespondingLeftSet(false)
        , m_isCorrespondingRightSet(false)

    {
    }

    Nullable<Length> legnth() const
    {
        return m_length;
    }

    void setLength(Length length)
    {
        m_length = length;
    }

    bool isCorrespondingLeftSet() const
    {
        return m_isCorrespondingLeftSet;
    }

    bool isCorrespondingRightSet() const
    {
        return m_isCorrespondingRightSet;
    }

    void markCorrespondingLeftIsSet()
    {
        m_isCorrespondingLeftSet = true;
    }

    void markCorrespondingRightIsSet()
    {
        m_isCorrespondingRightSet = true;
    }

    bool operator==(const LengthInlineDirectionAwereData& o)
    {
        return m_length == o.m_length;
    }

    bool operator!=(const LengthInlineDirectionAwereData& o)
    {
        return !operator==(o);
    }

private:
    Nullable<Length> m_length;
    bool m_isCorrespondingLeftSet = false;
    bool m_isCorrespondingRightSet = false;
};

// TODO: LengthBlockDirectionAwereData

} // namespace Starfish

#endif
