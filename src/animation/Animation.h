/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include <sys/time.h>
#include <Elementary.h>

#include "animation/CubicBezier.h"

namespace StarFish {
class Node;

enum TransitionTimingFunction {
    Ease,      // cubic-bezier(0, 0, 1, 1)
    Linear,    // cubic-bezier(0, 0, 1, 1)
    EaseIn,    // cubic-bezier(0.42, 0, 1, 1)
    EaseOut,   // cubic-bezier(0, 0, 0.58, 1)
    EaseInOut, // cubic-bezier(0.42, 0, 0.58, 1)
    StepStart,
    StepEnd,
    Custom
};

class AnimatedValue : public gc {
    enum VAULETYPE { UNDEFINED, COLOR, LENGTH, FLOAT, INT };

public:
    AnimatedValue()
    {
        m_type = UNDEFINED;
    }

    AnimatedValue(Color colorValue)
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

    virtual bool isColor()
    {
        return m_type == COLOR;
    }

    virtual bool isLength()
    {
        return m_type == LENGTH;
    }

    virtual bool isFloat()
    {
        return m_type == FLOAT;
    }

    virtual bool isInt()
    {
        return m_type == INT;
    }

    Color getColor()
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

protected:
    union ValueData {
        Color m_color;
        Length m_length;
        float m_float;
        int m_int;
        ValueData()
        {
        }
    } m_data;
    VAULETYPE m_type;
};

class AnimationTask : public gc {
public:
    static const int THRESHOLD_TICK = 10;
    AnimationTask(Node* target, CSSStyleValuePair::KeyKind targetProperty,
                  AnimatedValue from, AnimatedValue to, float duration,
                  float delay, CubicBeizer* cubicBezier);
    float progress();
    bool canExecute();
    bool isExpired()
    {
        return m_isExpired;
    }
    void update();
    virtual void execute() = 0;
    Node* node()
    {
        return m_targetElement;
    }
    CSSStyleValuePair::KeyKind propertyType()
    {
        return m_property;
    }

protected:
    AnimatedValue m_fromValue;
    AnimatedValue m_toValue;
    CSSStyleValuePair::KeyKind m_property;

private:
    bool m_isExpired;
    size_t m_startTimeMs;
    size_t m_lastModifiedTimeMs;
    size_t m_durationMs;
    size_t m_delayMs;
    Node* m_targetElement;
    CubicBeizer* m_cubicBezier;
};

class ColorAnimationTask : public AnimationTask {
public:
    ColorAnimationTask(Node* target, CSSStyleValuePair::KeyKind targetProperty,
                       AnimatedValue fromValue, AnimatedValue toValue,
                       float duration, float delay, CubicBeizer* cubicBezier)
        : AnimationTask(target, targetProperty, fromValue, toValue, duration,
                        delay, cubicBezier)
    {
    }
    void execute();
};

class LengthAnimationTask : public AnimationTask {
public:
    LengthAnimationTask(Node* target, CSSStyleValuePair::KeyKind targetProperty,
                        AnimatedValue fromValue, AnimatedValue toValue,
                        float duration, float delay, CubicBeizer* cubicBezier)
        : AnimationTask(target, targetProperty, fromValue, toValue, duration,
                        delay, cubicBezier)
    {
    }
    void execute();
};

class AnimationExecutor : public gc {
public:
    AnimationExecutor(Window* window)
        : m_isAlive(false)
        , m_platformAnimator(nullptr)
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
    void cancelPreviousAnimation(Node* target,
                                 CSSStyleValuePair::KeyKind cssType);
    void cancelAnimation(Node* target);
    void startIfNeeds();
    void stop();
    void stopIfNeeds();
    void step();

private:
    Window* m_window;
    bool m_isAlive;
    Ecore_Animator* m_platformAnimator;
    GCVector<AnimationTask*> m_animationList;
    CubicBeizer* m_timingFunctionPreset[5];
};
}
#endif
