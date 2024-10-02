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

#ifndef __StarfishFlowRelativeLengthData__
#define __StarfishFlowRelativeLengthData__

namespace Starfish {

class ComputedStyle;

class FlowRelativeLengthData : public gc {
public:
    FlowRelativeLengthData()
        : m_length(nullptr)
    {
    }

    FlowRelativeLengthData(Length length)
        : m_length(length)
    {
    }

    Optional<Length> legnth() const
    {
        return m_length;
    }

    void setLength(Length length)
    {
        m_length = length;
    }

protected:
    Optional<Length> m_length;
};

class FlowRelativeLengthInlineData : public FlowRelativeLengthData {
public:
    FlowRelativeLengthInlineData()
        : FlowRelativeLengthData()
        , m_isCorrespondingLeftSet(false)
        , m_isCorrespondingRightSet(false)

    {
    }

    FlowRelativeLengthInlineData(Length length)
        : FlowRelativeLengthData(length)
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

    bool operator==(const FlowRelativeLengthInlineData& o)
    {
        return m_length == o.m_length;
    }

    bool operator!=(const FlowRelativeLengthInlineData& o)
    {
        return !operator==(o);
    }

private:
    bool m_isCorrespondingLeftSet = false;
    bool m_isCorrespondingRightSet = false;
};

class FlowRelativeLengthBlockData : public FlowRelativeLengthData {
public:
    FlowRelativeLengthBlockData()
        : FlowRelativeLengthData()
        , m_isCorrespondingTopSet(false)
        , m_isCorrespondingBottomSet(false)

    {
    }

    FlowRelativeLengthBlockData(Length length)
        : FlowRelativeLengthData(length)
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

    bool operator==(const FlowRelativeLengthBlockData& o)
    {
        return m_length == o.m_length;
    }

    bool operator!=(const FlowRelativeLengthBlockData& o)
    {
        return !operator==(o);
    }

private:
    bool m_isCorrespondingTopSet = false;
    bool m_isCorrespondingBottomSet = false;
};

} // namespace Starfish

#endif
