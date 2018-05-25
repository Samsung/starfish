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

#ifndef __StarFishShadowData__
#define __StarFishShadowData__

namespace StarFish {
class CanvasShadowData;
class CanvasShadowDataList;
class ValueList;
class ShadowData : public gc {
public:
    ShadowData()
        : m_offsetX(Length::Fixed, 0)
        , m_offsetY(Length::Fixed, 0)
        , m_radius(Length::Fixed, 0)
        , m_spreadDistance(Length::Fixed, 0)
        , m_hasColor(false)
        , m_inset(false)
    {
    }

    void setLengths(ValueList* lengths);

    Length offsetX() const
    {
        return m_offsetX;
    }

    void setOffsetX(Length offsetX)
    {
        m_offsetX = offsetX;
    }

    Length offsetY() const
    {
        return m_offsetY;
    }

    void setOffsetY(Length offsetY)
    {
        m_offsetY = offsetY;
    }

    Length radius() const
    {
        return m_radius;
    }

    void setRadius(Length radius)
    {
        m_radius = radius;
    }

    Length spreadDistance() const
    {
        return m_spreadDistance;
    }

    void setSpreadDistance(Length spreadDistance)
    {
        m_spreadDistance = spreadDistance;
    }

    Unit::Color color() const
    {
        return m_color;
    }

    void setColor(Unit::Color color)
    {
        m_hasColor = true;
        m_color = color;
    }

    bool hasColor() const
    {
        return m_hasColor;
    }

    bool inset() const
    {
        return m_inset;
    }

    void setInset()
    {
        m_inset = true;
    }
    CanvasShadowData toCanvasShadowData(Frame* owner) const;

    bool operator==(const ShadowData& o) const
    {
        return ((this->m_offsetX == o.m_offsetX) &&
                (this->m_offsetY == o.m_offsetY) &&
                (this->m_radius == o.m_radius) && (this->m_color == o.m_color));
    }

    bool operator!=(const ShadowData& o) const
    {
        return !operator==(o);
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(ShadowData));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ShadowData)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ShadowData, m_offsetX));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ShadowData, m_offsetY));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ShadowData, m_radius));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(ShadowData, m_spreadDistance));
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
    Length m_spreadDistance;
    Unit::Color m_color;
    bool m_hasColor;
    bool m_inset;
};

class ShadowDataList : public GCVector<ShadowData> {
public:
    CanvasShadowDataList toCanvasShadowDataList(Frame* owner) const;

    bool operator==(const ShadowDataList& o) const
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
    bool operator!=(const ShadowDataList& o) const
    {
        return !operator==(o);
    }
};
}
#endif
