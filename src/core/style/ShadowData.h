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
class CanvasShadowData;
class CanvasShadowDataList;
class ShadowData : public gc {
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

    CanvasShadowData toCanvasShadowData(Frame* owner);

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

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ShadowData)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ShadowData, m_offsetX));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ShadowData, m_offsetY));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ShadowData, m_radius));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ShadowData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new(size_t size, ShadowData* shadow)
    {
        return shadow;
    }
    void* operator new[](size_t size) = delete;

private:
    Length m_offsetX;
    Length m_offsetY;
    Length m_radius;
    Unit::Color m_color;
    bool m_hasColor;
};

class ShadowDataList : public GCVector<ShadowData> {
public:
    CanvasShadowDataList toCanvasShadowDataList(Frame* owner);

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
