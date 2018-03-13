/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishTextDecorationData__
#define __StarFishTextDecorationData__

#include "core/style/Unit.h"

namespace StarFish {
class ComputedStyle;

class TextDecorationData {
public:
    TextDecorationData()
        : m_hasUnderLine(false)
        , m_hasLineThrough(false)
    {
    }

    TextDecorationData(TextDecorationData* other)
        : m_hasUnderLine(other->m_hasUnderLine)
        , m_hasLineThrough(other->m_hasLineThrough)
        , m_underLineColor(other->m_underLineColor)
        , m_lineThroughColor(other->m_lineThroughColor)
    {
    }

    void merge(ComputedStyle* style);

    void reset()
    {
        m_hasUnderLine = false;
        m_hasLineThrough = false;
    }

    bool hasUnderLine()
    {
        return m_hasUnderLine;
    }

    bool hasLineThrough()
    {
        return m_hasLineThrough;
    }

    Unit::Color underLineColor()
    {
        return m_underLineColor;
    }

    Unit::Color lineThroughColor()
    {
        return m_lineThroughColor;
    }

    void setUnderLineColor(Unit::Color color)
    {
        m_underLineColor = color;
    }

    void setLineThroughColor(Unit::Color color)
    {
        m_lineThroughColor = color;
    }

private:
    bool m_hasUnderLine;
    bool m_hasLineThrough;
    Unit::Color m_underLineColor;
    Unit::Color m_lineThroughColor;
};
}

#endif
