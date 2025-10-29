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
class StyleTransformOrigin;
class FilterFunction;

class AnimatedValue : public gc {
    enum class ValueType ENSURE_ENUM_UNSIGNED {
        Undefined,
        Color,
        LayoutUnit,
        Length,
        LengthSize,
        Float,
        Int,
        Matrix,
        TransformData,
        TransformOriginData,
        Visibility,
        Filter,
        AnimateMotion
    };

public:
    static Optional<AnimatedValue*> create(
        ComputedStyle* style, Element* element,
        const CSSStyleValuePair& property,
        const CSSStyleValuePair::KeyKind& keyKind, size_t layer,
        bool neededOriginProperty);

    static Optional<AnimatedValue*> createForSVGAnimation(
        Element* element, const CSSStyleValuePair& property,
        const CSSStyleValuePair::KeyKind& keyKind);

    static Optional<AnimatedValue*> createAnimatedValueFromColor(
        const CSSStyleValuePair& property);

    static Optional<AnimatedValue*> createAnimatedValueFromLength(
        const CSSStyleValuePair& property);

    static Optional<AnimatedValue*> createAnimatedValueFromBackgroundPosition(
        const CSSStyleValuePair& property, size_t layer);

    static Optional<AnimatedValue*> createAnimatedValueFromBackgroundSize(
        const CSSStyleValuePair& property, size_t layer);

    static Optional<AnimatedValue*> createAnimatedValueFromFilter(
        const CSSStyleValuePair& property);

    static Optional<AnimatedValue*> createAnimatedValueFromAnimateMotion(
        const CSSStyleValuePair& property);

    AnimatedValue()
    {
        m_type = ValueType::Undefined;
    }

    AnimatedValue(Unit::Color colorValue)
    {
        m_data.m_color = colorValue;
        m_type = ValueType::Color;
    }

    AnimatedValue(Length lengthValue)
    {
        m_data.m_length = lengthValue;
        m_type = ValueType::Length;
    }

    AnimatedValue(const LengthSize& size)
    {
        m_data.m_lengthSize = new LengthSize(size);
        m_type = ValueType::LengthSize;
    }

    AnimatedValue(LayoutUnit v)
    {
        m_data.m_layoutUnit = v;
        m_type = ValueType::LayoutUnit;
    }

    AnimatedValue(float floatValue)
    {
        m_data.m_float = floatValue;
        m_type = ValueType::Float;
    }

    AnimatedValue(int intValue)
    {
        m_data.m_int = intValue;
        m_type = ValueType::Int;
    }

    AnimatedValue(const SkMatrix& matrix)
    {
        m_data.m_matrix = matrix;
        m_type = ValueType::Matrix;
    }

    AnimatedValue(StyleTransformDataGroup* transform)
    {
        STARFISH_ASSERT(transform != nullptr);

        m_data.m_transformData = transform;
        m_type = ValueType::TransformData;
    }

    AnimatedValue(StyleTransformOrigin* transformOrigin)
    {
        STARFISH_ASSERT(transformOrigin);
        m_data.m_transformOriginData = transformOrigin;
        m_type = ValueType::TransformOriginData;
    }

    AnimatedValue(VisibilityValue v)
    {
        m_data.m_visibilityValue = v;
        m_type = ValueType::Visibility;
    }

    AnimatedValue(FilterFunction* v)
    {
        m_data.m_filterFunction = v;
        m_type = ValueType::Filter;
    }

    AnimatedValue(GCAtomicVector<Unit::FloatPoint>* v)
    {
        m_data.m_animateMotion = v;
        m_type = ValueType::AnimateMotion;
    }

    bool isColor() const
    {
        return m_type == ValueType::Color;
    }

    bool isLength() const
    {
        return m_type == ValueType::Length;
    }

    bool isLengthSize() const
    {
        return m_type == ValueType::LengthSize;
    }

    bool isFloat() const
    {
        return m_type == ValueType::Float;
    }

    bool isInt() const
    {
        return m_type == ValueType::Int;
    }

    bool isMatrix() const
    {
        return m_type == ValueType::Matrix;
    }

    bool isLayoutUnit() const
    {
        return m_type == ValueType::LayoutUnit;
    }

    bool isTransformData() const
    {
        return m_type == ValueType::TransformData;
    }

    bool isTransformOriginData() const
    {
        return m_type == ValueType::TransformOriginData;
    }

    bool isFilter() const
    {
        return m_type == ValueType::Filter;
    }

    bool isAnimateMotion() const
    {
        return m_type == ValueType::AnimateMotion;
    }

    Unit::Color getColor() const
    {
        STARFISH_ASSERT(m_type == ValueType::Color);
        return m_data.m_color;
    }

    Length getLength() const
    {
        STARFISH_ASSERT(m_type == ValueType::Length);
        return m_data.m_length;
    }

    void setLength(const Length& l)
    {
        STARFISH_ASSERT(m_type == ValueType::Length);
        m_data.m_length = l;
    }

    LengthSize* getLengthSize() const
    {
        STARFISH_ASSERT(m_type == ValueType::LengthSize);
        return m_data.m_lengthSize;
    }

    LayoutUnit getLayoutUnit() const
    {
        STARFISH_ASSERT(m_type == ValueType::LayoutUnit);
        return m_data.m_layoutUnit;
    }

    float getFloat() const
    {
        STARFISH_ASSERT(m_type == ValueType::Float);
        return m_data.m_float;
    }

    int getInt() const
    {
        STARFISH_ASSERT(m_type == ValueType::Int);
        return m_data.m_int;
    }

    SkMatrix getMatrix() const
    {
        STARFISH_ASSERT(m_type == ValueType::Matrix);
        return m_data.m_matrix;
    }

    StyleTransformDataGroup* getTransformData() const
    {
        STARFISH_ASSERT(m_type == ValueType::TransformData);
        return m_data.m_transformData;
    }

    StyleTransformOrigin* getTrasnformOriginData() const
    {
        STARFISH_ASSERT(m_type == ValueType::TransformOriginData);
        return m_data.m_transformOriginData;
    }

    VisibilityValue getVisibilityValue() const
    {
        STARFISH_ASSERT(m_type == ValueType::Visibility);
        return m_data.m_visibilityValue;
    }

    FilterFunction* getFilterValue() const
    {
        STARFISH_ASSERT(m_type == ValueType::Filter);
        return m_data.m_filterFunction;
    }

    GCAtomicVector<Unit::FloatPoint>* getAnimateMotionValue() const
    {
        STARFISH_ASSERT(m_type == ValueType::AnimateMotion);
        return m_data.m_animateMotion;
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
        StyleTransformOrigin* m_transformOriginData;
        VisibilityValue m_visibilityValue;
        FilterFunction* m_filterFunction;
        GCAtomicVector<Unit::FloatPoint>* m_animateMotion;
        ValueData()
            : m_int(0)
        {
        }
    } m_data;
    ValueType m_type;
};

} // namespace Starfish

#endif
