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
                     const Unit::Color& color, const bool& hasColor)
        : m_offsetX(offsetX)
        , m_offsetY(offsetY)
        , m_radius(radius)
        , m_spreadDistance(spreadDistance)
        , m_color(color)
        , m_hasColor(hasColor)
    {
    }

    float offsetX()
    {
        return m_offsetX;
    }

    float offsetY()
    {
        return m_offsetY;
    }

    float radius()
    {
        return m_radius;
    }

    Unit::Color color()
    {
        return m_color;
    }

    bool hasColor()
    {
        return m_hasColor;
    }

    float spreadDistance()
    {
        return m_spreadDistance;
    }

private:
    float m_offsetX;
    float m_offsetY;
    float m_radius;
    float m_spreadDistance;
    Unit::Color m_color;
    bool m_hasColor;
};

class CanvasShadowDataList : public GCVector<CanvasShadowData> {
public:
};
}
#endif
