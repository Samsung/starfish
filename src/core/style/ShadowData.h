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

#ifndef __StarFishShadowData__
#define __StarFishShadowData__

namespace StarFish {

class ShadowData {
public:
    ShadowData()
        : m_hasColor(false)
    {
    }

    Length offsetX()
    {
        return m_offsetX;
    }
    void setOffsetX(Length offsetX)
    {
        m_offsetX = offsetX;
    }

    Length offsetY()
    {
        return m_offsetY;
    }
    void setOffsetY(Length offsetY)
    {
        m_offsetY = offsetY;
    }

    Length radius()
    {
        return m_radius;
    }
    void setRadius(Length radius)
    {
        m_radius = radius;
    }

    Unit::Color color()
    {
        return m_color;
    }
    void setColor(Unit::Color color)
    {
        m_hasColor = true;
        m_color = color;
    }
    bool hasColor()
    {
        return m_hasColor;
    }

    bool operator==(const ShadowData& o)
    {
        return ((this->m_offsetX == o.m_offsetX) &&
                (this->m_offsetY == o.m_offsetY) &&
                (this->m_radius == o.m_radius) && (this->m_color == o.m_color));
    }

    bool operator!=(const ShadowData& o)
    {
        return !operator==(o);
    }

private:
    Length m_offsetX;
    Length m_offsetY;
    Length m_radius;
    Unit::Color m_color;
    bool m_hasColor;
};

class ShadowDataList : public GCVector<ShadowData> {
public:
    bool operator==(const ShadowDataList& o)
    {
        if (this->size() != o.size()) {
            return false;
        }
        for (size_t i = 0; i < this->size(); i++) {
            if (this->at(i) != o.at(i)) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const ShadowDataList& o)
    {
        return !operator==(o);
    }
};
}
#endif
