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

namespace StarFish {

class Node;
class PlatformWindow;

class AnimatedValue : public gc {
    enum ValueType { UNDEFINED, COLOR, LENGTH, FLOAT, INT, MATRIX };

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

    bool isColor()
    {
        return m_type == COLOR;
    }

    bool isLength()
    {
        return m_type == LENGTH;
    }

    bool isFloat()
    {
        return m_type == FLOAT;
    }

    bool isInt()
    {
        return m_type == INT;
    }

    bool isMatrix()
    {
        return m_type == MATRIX;
    }

    Unit::Color getColor()
    {
        STARFISH_ASSERT(m_type == COLOR);
        return m_data.m_color;
    }

    Length getLength()
    {
        STARFISH_ASSERT(m_type == LENGTH);
        return m_data.m_length;
    }

    float getFloat()
    {
        STARFISH_ASSERT(m_type == FLOAT);
        return m_data.m_float;
    }

    int getInt()
    {
        STARFISH_ASSERT(m_type == INT);
        return m_data.m_int;
    }

    SkMatrix getMatrix()
    {
        STARFISH_ASSERT(m_type == MATRIX);
        return m_data.m_matrix;
    }

    void* operator new(size_t size)
    {
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
        float m_float;
        int m_int;
        SkMatrix m_matrix;
        ValueData()
        {
        }
    } m_data;
    ValueType m_type;
};

class AnimationTask : public gc {
    friend class AnimationExecutor;

public:
    AnimationTask(Element* target, CSSStyleValuePair::KeyKind targetProperty,
                  String* targetPropertyString, AnimatedValue from,
                  AnimatedValue to, float durationInms, float delayInms,
                  AnimationTimingFunction* timingFunction);
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
    CSSStyleValuePair::KeyKind propertyType()
    {
        return m_property;
    }

    Element* targetElement() const
    {
        return m_targetElement;
    }

protected:
    AnimatedValue m_fromValue;
    AnimatedValue m_toValue;
    CSSStyleValuePair::KeyKind m_property;

private:
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
                       String* targetPropertyString, AnimatedValue fromValue,
                       AnimatedValue toValue, float duration, float delay,
                       AnimationTimingFunction* timingFunction)
        : AnimationTask(target, targetProperty, targetPropertyString, fromValue,
                        toValue, duration, delay, timingFunction)
    {
    }
    void execute(float progress) override;
};

class LengthAnimationTask : public AnimationTask {
public:
    LengthAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        String* targetPropertyString, AnimatedValue fromValue,
                        AnimatedValue toValue, float duration, float delay,
                        AnimationTimingFunction* timingFunction)
        : AnimationTask(target, targetProperty, targetPropertyString, fromValue,
                        toValue, duration, delay, timingFunction)
    {
    }
    void execute(float progress) override;
};

class TransformAnimationTask : public AnimationTask {
public:
    TransformAnimationTask(Element* target,
                           CSSStyleValuePair::KeyKind targetProperty,
                           String* targetPropertyString,
                           AnimatedValue fromValue, float duration, float delay,
                           AnimationTimingFunction* timingFunction);
    void execute(float progress) override;
    void attachedToElement() override;
    void detachedFromElement() override;
    void computeToValue();

private:
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

    void registerAnimation(AnimationTask* newtask);
    void cancelPreviousAnimation(Element* target,
                                 CSSStyleValuePair::KeyKind cssType);
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
