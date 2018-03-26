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

#ifndef __StarFishCanvasShadowData__
#define __StarFishCanvasShadowData__

namespace StarFish {

class CanvasShadowData {
public:
    CanvasShadowData(const float& offsetX, const float& offsetY,
                     const float& radius, const float& spreadDistance,
                     const Unit::Color& color, const bool& hasColor,
                     const bool& inset)
        : m_offsetX(offsetX)
        , m_offsetY(offsetY)
        , m_radius(radius)
        , m_spreadDistance(spreadDistance)
        , m_color(color)
        , m_hasColor(hasColor)
        , m_inset(inset)
    {
    }

    float offsetX() const
    {
        return m_offsetX;
    }

    float offsetY() const
    {
        return m_offsetY;
    }

    float radius() const
    {
        return m_radius;
    }

    Unit::Color color() const
    {
        return m_color;
    }

    bool hasColor() const
    {
        return m_hasColor;
    }

    float spreadDistance() const
    {
        return m_spreadDistance;
    }

    bool inset() const
    {
        return m_inset;
    }

private:
    float m_offsetX;
    float m_offsetY;
    float m_radius;
    float m_spreadDistance;
    Unit::Color m_color;
    bool m_hasColor;
    bool m_inset;
};

class CanvasShadowDataList : public GCVector<CanvasShadowData> {
public:
};
}
#endif
