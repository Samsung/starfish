/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAnimatedValue__
#define __StarfishAnimatedValue__

#include <SkMatrix.h>

#include "core/style/Style.h"

namespace Starfish {

class Node;
class StyleTransformDataGroup;
class TimingFunction;

class AnimatedValue : public gc {
    enum ValueType ENSURE_ENUM_UNSIGNED {
        UNDEFINED,
        COLOR,
        LAYOUT_UNIT,
        LENGTH,
        LENGTH_SIZE,
        FLOAT,
        INT,
        MATRIX,
        TRANSFORM_DATA,
        VISIBILITY
    };

public:
    static Optional<AnimatedValue*> create(
        ComputedStyle* style, Element* element,
        const CSSStyleValuePair& property,
        const CSSStyleValuePair::KeyKind& keyKind, size_t layer,
        bool neededOriginProperty);

    static Optional<AnimatedValue*> createAnimatedValueFromColor(
        const CSSStyleValuePair& property);

    static Optional<AnimatedValue*> createAnimatedValueFromLength(
        const CSSStyleValuePair& property);

    static Optional<AnimatedValue*> createAnimatedValueFromBackgroundPosition(
        const CSSStyleValuePair& property, size_t layer);

    static Optional<AnimatedValue*> createAnimatedValueFromBackgroundSize(
        const CSSStyleValuePair& property, size_t layer);

    AnimatedValue()
    {
        m_type = UNDEFINED;
    }

    AnimatedValue(Unit::Color colorValue)
    {
        m_data.m_color = colorValue;
        m_type = COLOR;
    }

    AnimatedValue(Length lengthValue)
    {
        m_data.m_length = lengthValue;
        m_type = LENGTH;
    }

    AnimatedValue(const LengthSize& size)
    {
        m_data.m_lengthSize = new LengthSize(size);
        m_type = LENGTH_SIZE;
    }

    AnimatedValue(LayoutUnit v)
    {
        m_data.m_layoutUnit = v;
        m_type = LAYOUT_UNIT;
    }

    AnimatedValue(float floatValue)
    {
        m_data.m_float = floatValue;
        m_type = FLOAT;
    }

    AnimatedValue(int intValue)
    {
        m_data.m_int = intValue;
        m_type = INT;
    }

    AnimatedValue(const SkMatrix& matrix)
    {
        m_data.m_matrix = matrix;
        m_type = MATRIX;
    }

    AnimatedValue(StyleTransformDataGroup* transform)
    {
        STARFISH_ASSERT(transform != nullptr);

        m_data.m_transformData = transform;
        m_type = TRANSFORM_DATA;
    }

    AnimatedValue(VisibilityValue v)
    {
        m_data.m_visibilityValue = v;
        m_type = VISIBILITY;
    }

    bool isColor() const
    {
        return m_type == COLOR;
    }

    bool isLength() const
    {
        return m_type == LENGTH;
    }

    bool isLengthSize() const
    {
        return m_type == LENGTH_SIZE;
    }

    bool isFloat() const
    {
        return m_type == FLOAT;
    }

    bool isInt() const
    {
        return m_type == INT;
    }

    bool isMatrix() const
    {
        return m_type == MATRIX;
    }

    bool isLayoutUnit() const
    {
        return m_type == LAYOUT_UNIT;
    }

    bool isTransformData() const
    {
        return m_type == TRANSFORM_DATA;
    }

    Unit::Color getColor() const
    {
        STARFISH_ASSERT(m_type == COLOR);
        return m_data.m_color;
    }

    Length getLength() const
    {
        STARFISH_ASSERT(m_type == LENGTH);
        return m_data.m_length;
    }

    void setLength(const Length& l)
    {
        STARFISH_ASSERT(m_type == LENGTH);
        m_data.m_length = l;
    }

    LengthSize* getLengthSize() const
    {
        STARFISH_ASSERT(m_type == LENGTH_SIZE);
        return m_data.m_lengthSize;
    }

    LayoutUnit getLayoutUnit() const
    {
        STARFISH_ASSERT(m_type == LAYOUT_UNIT);
        return m_data.m_layoutUnit;
    }

    float getFloat() const
    {
        STARFISH_ASSERT(m_type == FLOAT);
        return m_data.m_float;
    }

    int getInt() const
    {
        STARFISH_ASSERT(m_type == INT);
        return m_data.m_int;
    }

    SkMatrix getMatrix() const
    {
        STARFISH_ASSERT(m_type == MATRIX);
        return m_data.m_matrix;
    }

    StyleTransformDataGroup* getTransformData() const
    {
        STARFISH_ASSERT(m_type == TRANSFORM_DATA);
        return m_data.m_transformData;
    }

    VisibilityValue getVisibilityValue() const
    {
        STARFISH_ASSERT(m_type == VISIBILITY);
        return m_data.m_visibilityValue;
    }

    void changeToFixedIfNeeded(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutUnit viewportWidth,
                               LayoutUnit viewportHeight,
                               Optional<ComputedStyle*> cs);

    inline void* operator new(size_t size, void* p)
    {
        STARFISH_ASSERT(p != nullptr);
        STARFISH_ASSERT(size == sizeof(AnimatedValue));
        return p;
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(AnimatedValue));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(AnimatedValue)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(AnimatedValue, m_data));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(AnimatedValue));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

#ifndef NDEBUG
    String* toString() const;
#endif

protected:
    union ValueData {
        Unit::Color m_color;
        Length m_length;
        LengthSize* m_lengthSize;
        LayoutUnit m_layoutUnit;
        float m_float;
        int m_int;
        SkMatrix m_matrix;
        StyleTransformDataGroup* m_transformData;
        VisibilityValue m_visibilityValue;
        ValueData()
            : m_int(0)
        {
        }
    } m_data;
    ValueType m_type;
};

} // namespace Starfish

#endif
