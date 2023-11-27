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

#ifndef __StarfishBorderDirectionAwereData__
#define __StarfishBorderDirectionAwereData__

#include "core/style/BorderValue.h"
#include "core/style/BorderImage.h"

namespace Starfish {

enum class BorderValueKind {
    kColor,
    kWidth,
    kStyle,
};

class BorderDirectionAwereData : public gc {
public:
    static bool damaged(const BorderDirectionAwereData* lhs,
                        const BorderDirectionAwereData* rhs, bool* damagedKeys);

    BorderDirectionAwereData()
    {
        m_borderValue.setWidth(Length());
    }

    BorderValue& borderValue()
    {
        return m_borderValue;
    }

    void setStyle(BorderStyleValue style)
    {
        m_borderValue.setStyle(style);
        m_hasStyle = true;
    }

    bool hasStlye()
    {
        return m_hasStyle;
    }

    void setWidth(Length length)
    {
        m_borderValue.setWidth(length);
        m_hasWidth = true;
    }

    bool hasWidth()
    {
        return m_hasWidth;
    }

    void setColor(Unit::Color color)
    {
        m_borderValue.setColor(color);
        m_hasColor = true;
    }

    bool hasColor()
    {
        return m_hasColor;
    }

protected:
    BorderValue m_borderValue;
    bool m_hasStyle = false;
    bool m_hasWidth = false;
    bool m_hasColor = false;
};

class BorderBlockDirectionAwereData : public BorderDirectionAwereData {
public:
    BorderBlockDirectionAwereData()
    {
    }

    bool isCorrespondingTopSpecifiedLater(BorderValueKind type) const
    {
        return m_isCorrespondingTopSpecifiedLater[static_cast<size_t>(type)];
    }

    bool isCorrespondingBottomSpecifiedLater(BorderValueKind type) const
    {
        return m_isCorrespondingBottomSpecifiedLater[static_cast<size_t>(type)];
    }

    void setCorrespondingTopIsSpecifiedLater(BorderValueKind type, bool value)
    {
        m_isCorrespondingTopSpecifiedLater[static_cast<size_t>(type)] = value;
    }

    void setCorrespondingBottomIsSpecifiedLater(BorderValueKind type,
                                                bool value)
    {
        m_isCorrespondingBottomSpecifiedLater[static_cast<size_t>(type)] =
            value;
    }

    bool operator==(const BorderBlockDirectionAwereData& o)
    {
        return m_borderValue == o.m_borderValue;
    }

    bool operator!=(const BorderBlockDirectionAwereData& o)
    {
        return !operator==(o);
    }

private:
    bool m_isCorrespondingTopSpecifiedLater[3] = { false };
    bool m_isCorrespondingBottomSpecifiedLater[3] = { false };
};

} // namespace Starfish

#endif
