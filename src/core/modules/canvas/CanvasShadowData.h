/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishCanvasShadowData__
#define __StarFishCanvasShadowData__

namespace StarFish {

class CanvasShadowData {
public:
    CanvasShadowData(float& offsetX, float& offsetY, float& radius,
                     Unit::Color& color, bool& hasColor)
        : m_offsetX(offsetX)
        , m_offsetY(offsetY)
        , m_radius(radius)
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

private:
    float m_offsetX;
    float m_offsetY;
    float m_radius;
    Unit::Color m_color;
    bool m_hasColor;
};

class CanvasShadowDataList : public GCVector<CanvasShadowData> {
public:
};
}
#endif
