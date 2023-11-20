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

class LengthDirectionAwereData : public gc {
public:
    LengthDirectionAwereData()
        : m_length(nullptr)
    {
    }

    LengthDirectionAwereData(Length length)
        : m_length(length)
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

protected:
    Nullable<Length> m_length;
};

class LengthInlineDirectionAwereData : public LengthDirectionAwereData {
public:
    LengthInlineDirectionAwereData()
        : LengthDirectionAwereData()
        , m_isCorrespondingLeftSet(false)
        , m_isCorrespondingRightSet(false)

    {
    }

    LengthInlineDirectionAwereData(Length length)
        : LengthDirectionAwereData(length)
        , m_isCorrespondingLeftSet(false)
        , m_isCorrespondingRightSet(false)

    {
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
    bool m_isCorrespondingLeftSet = false;
    bool m_isCorrespondingRightSet = false;
};

class LengthBlockDirectionAwereData : public LengthDirectionAwereData {
public:
    LengthBlockDirectionAwereData()
        : LengthDirectionAwereData()
        , m_isCorrespondingTopSet(false)
        , m_isCorrespondingBottomSet(false)

    {
    }

    LengthBlockDirectionAwereData(Length length)
        : LengthDirectionAwereData(length)
        , m_isCorrespondingTopSet(false)
        , m_isCorrespondingBottomSet(false)

    {
    }

    bool isCorrespondingTopSet() const
    {
        return m_isCorrespondingTopSet;
    }

    bool isCorrespondingBottomSet() const
    {
        return m_isCorrespondingBottomSet;
    }

    void markCorrespondingTopIsSet()
    {
        m_isCorrespondingTopSet = true;
    }

    void markCorrespondingBottomIsSet()
    {
        m_isCorrespondingBottomSet = true;
    }

    bool operator==(const LengthBlockDirectionAwereData& o)
    {
        return m_length == o.m_length;
    }

    bool operator!=(const LengthBlockDirectionAwereData& o)
    {
        return !operator==(o);
    }

private:
    bool m_isCorrespondingTopSet = false;
    bool m_isCorrespondingBottomSet = false;
};

} // namespace Starfish

#endif
