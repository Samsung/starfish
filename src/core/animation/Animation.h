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
public:
    static const int THRESHOLD_TICK = 10;
    AnimationTask(Element* target, CSSStyleValuePair::KeyKind targetProperty,
                  String* targetPropertyString, AnimatedValue from,
                  AnimatedValue to, float durationInms, float delayInms,
                  AnimationTimingFunction* timingFunction);
    float progress();
    bool canExecute();
    bool isExpired()
    {
        return m_isExpired;
    }
    void update();
    void fireStartEventIfNeeds();
    void fireEndEvent();
    void fireCancelEvent();
    virtual void execute()
    {
        m_isExpired = true;
    }
    virtual void attachedToElement()
    {
    }
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
    bool m_isExpired;
    bool m_isStarted;
    size_t m_startTimeMs;
    size_t m_lastModifiedTimeMs;
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
    void execute();
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
    void execute();
};

class TransformAnimationTask : public AnimationTask {
public:
    TransformAnimationTask(Element* target,
                           CSSStyleValuePair::KeyKind targetProperty,
                           String* targetPropertyString,
                           AnimatedValue fromValue, float duration, float delay,
                           AnimationTimingFunction* timingFunction)
        : AnimationTask(
              target, CSSStyleValuePair::KeyKind::Transform,
              transitionPropertyValueToString(TransitionPropertyTransformValue),
              fromValue, AnimatedValue(), duration, delay, timingFunction)
    {
    }
    void setup();
    void execute() override;
    void attachedToElement() override;
    void detachedFromElement() override;
    void computeToValue();

private:
};

class AnimationExecutor : public gc {
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

private:
    bool m_isAlive;
    Window* m_window;
    size_t m_platformAnimator;
    GCVector<AnimationTask*> m_animationList;
};
}
#endif
