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

#ifndef __StarfishAnimation__
#define __StarfishAnimation__

#include "core/style/Style.h"
#include "core/style/StyleBackgroundData.h"

namespace Starfish {

class Node;
class PlatformWindow;
class StyleTransformDataGroup;
class AnimationTimingFunction;

class AnimatedValue : public gc {
    enum ValueType ENSURE_ENUM_UNSIGNED {
        UNDEFINED,
        COLOR,
        LAYOUT_UNIT,
        LENGTH,
        LENGTH_SIZE,
        FLOAT,
        INT,
        MATRIX
    };

public:
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
        ValueData()
            : m_int(0)
        {
        }
    } m_data;
    ValueType m_type;
};

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* oldStyle, Frame* oldFrame,
    ComputedStyle* newStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, float>>&
        canceledAnimationProgress); // returns true if animation registered

class ActiveAnimationTask : public gc {
public:
    ActiveAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        const AnimatedValue& from, const AnimatedValue& to,
                        uint64_t durationInms, uint64_t delayInms,
                        AnimationTimingFunction* timingFunction);

    virtual ~ActiveAnimationTask()
    {
    }

    CSSStyleValuePair::KeyKind property() const
    {
        return m_property;
    }

    Element* targetElement() const
    {
        return m_targetElement;
    }

    AnimatedValue fromValue() const
    {
        return m_fromValue;
    }

    AnimatedValue toValue() const
    {
        return m_toValue;
    }

    void step(uint64_t tickCount, ComputedStyle* style);
    virtual void execute(float progress, ComputedStyle* style)
    {
    }
    virtual bool taskCanContinue(ComputedStyle* newStyle)
    {
        return false;
    }

    virtual void attachToElement(ComputedStyle* style)
    {
    }

    virtual void detachFromElement(ComputedStyle* style)
    {
    }

    virtual bool isKindOfTransitionProperty(CSSStyleValuePair::KeyKind k)
    {
        return k == m_property;
    }

    float fraction(uint64_t tickCount) const
    {
        if (tickCount < m_startTimeMs) {
            return 0;
        }
        uint64_t timeDiff = tickCount - m_startTimeMs;
        float result = timeDiff / ((float)m_durationMs);
        return std::min(result, 1.0f);
    }

    void fireStartEvent();
    void fireEndEvent();
    void fireCancelEvent();

    void updateDuration(uint64_t d)
    {
        m_durationMs = d;
    }

    uint64_t duration()
    {
        return m_durationMs;
    }

    void initializeStartTimeIfNeeded(uint64_t d)
    {
        if (m_startTimeMs == 0) {
            m_startTimeMs = d;
        }
    }

protected:
    float computeProgress(float fraction);

    CSSStyleValuePair::KeyKind m_property;
    Element* m_targetElement;

    AnimatedValue m_fromValue;
    AnimatedValue m_toValue;

    uint64_t m_startTimeMs;
    uint64_t m_durationMs;
    uint64_t m_delayMs;
    AnimationTimingFunction* m_timingFunction;
};

class ActiveOpacityAnimationTask : public ActiveAnimationTask {
public:
    ActiveOpacityAnimationTask(Element* target,
                               CSSStyleValuePair::KeyKind targetProperty,
                               const AnimatedValue& from,
                               const AnimatedValue& to, uint64_t durationInms,
                               uint64_t delayInms,
                               AnimationTimingFunction* timingFunction)
        : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                              delayInms, timingFunction)
    {
    }
    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual void attachToElement(ComputedStyle* style) override;
    virtual void detachFromElement(ComputedStyle* style) override;
};

class ActiveTransformAnimationTask : public ActiveAnimationTask {
public:
    struct MatrixDecomposed2D {
        float translateX;
        float translateY;
        float scaleX;
        float scaleY;
        float angle;
        float matrixM11;
        float matrixM12;
        float matrixM21;
        float matrixM22;

        MatrixDecomposed2D()
            : translateX(0)
            , translateY(0)
            , scaleX(0)
            , scaleY(0)
            , angle(0)
            , matrixM11(0)
            , matrixM12(0)
            , matrixM21(0)
            , matrixM22(0)

        {
        }
    };

    ActiveTransformAnimationTask(Element* target,
                                 CSSStyleValuePair::KeyKind targetProperty,
                                 const AnimatedValue& from,
                                 const AnimatedValue& to, uint64_t durationInms,
                                 uint64_t delayInms,
                                 AnimationTimingFunction* timingFunction,
                                 StyleTransformDataGroup* orgTransformValue);
    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual void attachToElement(ComputedStyle* style) override;
    virtual void detachFromElement(ComputedStyle* style) override;

protected:
    StyleTransformDataGroup* m_originalTransformValue;
    MatrixDecomposed2D m_decomposedFrom;
    MatrixDecomposed2D m_decomposedTo;
};

class ActiveColorAnimationTask : public ActiveAnimationTask {
public:
    ActiveColorAnimationTask(Element* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             const AnimatedValue& from, const AnimatedValue& to,
                             uint64_t durationInms, uint64_t delayInms,
                             AnimationTimingFunction* timingFunction)
        : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                              delayInms, timingFunction)
    {
    }
    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
};

class ActiveLengthAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthAnimationTask(Element* target,
                              CSSStyleValuePair::KeyKind targetProperty,
                              const AnimatedValue& from,
                              const AnimatedValue& to, uint64_t durationInms,
                              uint64_t delayInms,
                              AnimationTimingFunction* timingFunction,
                              Length originalToValue,
                              size_t indexForBgLayer = 0);
    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind k) override;

protected:
    Length m_originalToValue;
    size_t m_indexForBgLayer;
};

class ActiveLengthSizeAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthSizeAnimationTask(Element* target,
                                  CSSStyleValuePair::KeyKind targetProperty,
                                  const AnimatedValue& from,
                                  const AnimatedValue& to,
                                  uint64_t durationInms, uint64_t delayInms,
                                  AnimationTimingFunction* timingFunction,
                                  LengthSize originalToValue,
                                  size_t indexForBgLayer = 0);
    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind k) override;

protected:
    LengthSize m_originalToValue;
    size_t m_indexForBgLayer;
};

class AnimationExecutor : public gc {
public:
    AnimationExecutor(Window* window)
        : m_window(window)
    {
        m_window = window;
    }

    Window* window()
    {
        return m_window;
    }

    GCVector<ActiveAnimationTask*>& activeAnimations()
    {
        return m_activeAnimations;
    }

    bool hasActiveAnimiation(Element* element, CSSStyleValuePair::KeyKind p)
    {
        bool found = false;
        for (size_t i = 0; i < m_activeAnimations.size(); i++) {
            if (m_activeAnimations[i]->targetElement() == element &&
                m_activeAnimations[i]->property() == p) {
                found = true;
                break;
            }
        }
        return found;
    }

    void registerAnimation(ActiveAnimationTask* a, ComputedStyle* style)
    {
        m_activeAnimations.push_back(a);
        a->attachToElement(style);
        a->fireStartEvent();
    }

    void checkActiveAnimationExecutorInWebView();

private:
    Window* m_window;
    GCVector<ActiveAnimationTask*> m_activeAnimations;
};
}
#endif
