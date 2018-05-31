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

#ifndef __StarFishAnimation__
#define __StarFishAnimation__

#include "core/animation/AnimationTimingFunction.h"
#include "core/style/Style.h"
#include "core/style/StyleBackgroundData.h"

namespace StarFish {

class Node;
class PlatformWindow;

class AnimatedValue : public gc {
    enum ValueType {
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

class AnimationTask : public gc {
    friend class AnimationExecutor;

public:
    AnimationTask(Element* target, CSSStyleValuePair::KeyKind targetProperty,
                  AnimatedValue from, AnimatedValue to, float durationInms,
                  float delayInms, AnimationTimingFunction* timingFunction,
                  void* data = nullptr);
    virtual ~AnimationTask()
    {
    }
    float computeProgress(uint64_t tickCount);
    bool canExecute();
    void fireStartEvent();
    void fireEndEvent();
    void fireCancelEvent();
    virtual void execute(float progress)
    {
    }
    virtual void attachedToElement();
    virtual void detachedFromElement()
    {
    }
    CSSStyleValuePair::KeyKind propertyType() const
    {
        return m_property;
    }

    Element* targetElement() const
    {
        return m_targetElement;
    }

    void* extraData() const
    {
        return m_extraData;
    }

protected:
    AnimatedValue m_fromValue;
    AnimatedValue m_toValue;
    CSSStyleValuePair::KeyKind m_property;
    void* m_extraData;

private:
    bool m_isStartEventFired;
    size_t m_startTimeMs;
    size_t m_durationMs;
    size_t m_delayMs;
    String* m_targetPropertyString;
    Element* m_targetElement;
    AnimationTimingFunction* m_timingFunction;
};

class ColorAnimationTask : public AnimationTask {
public:
    ColorAnimationTask(Element* target,
                       CSSStyleValuePair::KeyKind targetProperty,
                       AnimatedValue fromValue, AnimatedValue toValue,
                       float duration, float delay,
                       AnimationTimingFunction* timingFunction,
                       void* data = nullptr)
        : AnimationTask(target, targetProperty, fromValue, toValue, duration,
                        delay, timingFunction, data)
    {
    }
    void execute(float progress) override;
};

class LengthAnimationTask : public AnimationTask {
public:
    LengthAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        AnimatedValue fromValue, AnimatedValue toValue,
                        float duration, float delay,
                        AnimationTimingFunction* timingFunction,
                        void* data = nullptr)
        : AnimationTask(target, targetProperty, fromValue, toValue, duration,
                        delay, timingFunction, data)
    {
    }
    void execute(float progress) override;
    void attachedToElement() override;
    void computeToValue();

protected:
    Length interpolateFixed(float progress) const;
    Length interpolateFixedOrPercent(float progress) const;

protected:
    LayoutUnit m_toFixedValue;
};

class LengthSizeAnimationTask : public AnimationTask {
public:
    LengthSizeAnimationTask(Element* target,
                            CSSStyleValuePair::KeyKind targetProperty,
                            AnimatedValue fromValue, AnimatedValue toValue,
                            float duration, float delay,
                            AnimationTimingFunction* timingFunction,
                            void* data = nullptr)
        : AnimationTask(target, targetProperty, fromValue, toValue, duration,
                        delay, timingFunction, data)
    {
    }
    void execute(float progress) override;
    void attachedToElement() override;

protected:
    LengthSize interpolateFixedOrPercent(float progress) const;
};

class OpacityAnimationTask : public AnimationTask {
public:
    OpacityAnimationTask(Element* target, AnimatedValue fromValue,
                         AnimatedValue toValue, float duration, float delay,
                         AnimationTimingFunction* timingFunction,
                         void* data = nullptr);
    void execute(float progress) override;
    void attachedToElement() override;
    void detachedFromElement() override;

private:
    void opacityUpdated(bool before, bool after);
};

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

class TransformAnimationTask : public AnimationTask {
public:
    TransformAnimationTask(Element* target, AnimatedValue fromValue,
                           float duration, float delay,
                           AnimationTimingFunction* timingFunction);
    void execute(float progress) override;
    void attachedToElement() override;
    void detachedFromElement() override;
    void computeToValue();

private:
    MatrixDecomposed2D m_decomposedFrom;
    MatrixDecomposed2D m_decomposedTo;
};

class AnimationExecutor : public gc {
    struct PendingAnimiationInfo : public gc {
        Element* element;
        ComputedStyle* oldStyle;
        ComputedStyle* newStyle;
        Frame* oldFrame;
    };

public:
    AnimationExecutor(Window* window)
        : m_isAlive(false)
        , m_platformAnimator(0)
    {
        m_window = window;
    }

    bool isAlive()
    {
        return m_isAlive;
    }

    Window* window()
    {
        return m_window;
    }

    void registerAnimation(AnimationTask* newTask);
    void cancelPreviousAnimationIfNeeded(AnimationTask* newTask);
    void cancelAnimation(Element* target);
    void startIfNeeds();
    void stop();
    void stopIfNeeds();
    void step();

    void runPendingAnimation();
    void addPendingAnimation(Element* element, ComputedStyle* oldStyle,
                             ComputedStyle* newStyle, Frame* oldFrame);

private:
    bool m_isAlive;
    Window* m_window;
    size_t m_platformAnimator;
    GCVector<AnimationTask*> m_animationList;
    GCVector<PendingAnimiationInfo*> m_pendingAnimationInfoList;
};
}
#endif
