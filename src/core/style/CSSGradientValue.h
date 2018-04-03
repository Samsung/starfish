/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCSSGradientValue__
#define __StarFishCSSGradientValue__

#include "core/style/Style.h"

namespace StarFish {

enum class CSSGradientType { LinearGradient, RadialGradient };

class ColorStop : public gc {
public:
    ColorStop()
        : m_color()
        , m_length()
    {
    }

    Unit::Color color()
    {
        return m_color;
    }

    void setColor(Unit::Color color)
    {
        m_color = color;
    }

    Length length()
    {
        return m_length;
    }

    void setLength(Length length)
    {
        m_length = length;
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ColorStop)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ColorStop, m_length));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ColorStop));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new(size_t size, ColorStop* colorStop)
    {
        return colorStop;
    }
    void* operator new[](size_t size) = delete;

private:
    Unit::Color m_color;
    Length m_length;
};

class CSSGradientValue : public gc {
public:
    CSSGradientValue(CSSGradientType gradientType)
        : m_gradientType(gradientType)
        , m_startingX(0)
        , m_startingY(0)
        , m_endingX(0)
        , m_endingY(0)
        , m_colorStopList()
    {
    }

    CSSGradientType type()
    {
        return m_gradientType;
    }

    float startingX()
    {
        return m_startingX;
    }

    void setStartingX(float val)
    {
        m_startingX = val;
    }

    float startingY()
    {
        return m_startingY;
    }

    void setStartingY(float val)
    {
        m_startingY = val;
    }

    float endingX()
    {
        return m_endingX;
    }

    void setEndingX(float val)
    {
        m_endingX = val;
    }

    float endingY()
    {
        return m_endingY;
    }

    void setEndingY(float val)
    {
        m_endingY = val;
    }

    String* toString();

    GCVector<ColorStop*>& colorStopList()
    {
        return m_colorStopList;
    }

protected:
    CSSGradientType m_gradientType;
    float m_startingX;
    float m_startingY;
    float m_endingX;
    float m_endingY;
    GCVector<ColorStop*> m_colorStopList;
};

class CSSLinearGradientValue : public CSSGradientValue {
public:
    CSSLinearGradientValue(CSSAngle angle = CSSAngle(180))
        : CSSGradientValue(CSSGradientType::LinearGradient)
        , m_angle(angle)
    {
    }

    CSSAngle angle()
    {
        return m_angle;
    }

    void setAngle(CSSAngle val)
    {
        m_angle = val;
    }

private:
    CSSAngle m_angle;
};

} /* namespace StarFish */

#endif /* __StarFishCSSGradientValue__ */
