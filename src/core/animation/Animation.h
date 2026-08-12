/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAnimation__
#define __StarfishAnimation__

#include "core/dom/EventTarget.h"

namespace Starfish {

struct EffectTiming {
    EffectTiming()
        : m_delay(0)
        , m_direction(String::emptyString)
        , m_duration(0.0)
        , m_easing(String::emptyString)
        , m_endDelay(0)
        , m_fill(String::emptyString)
        , m_iterationStart(0.0)
        , m_iterations(1.0)
    {
    }

    void setDelay(double d)
    {
        m_delay = d;
    }
    void setEndDelay(double d)
    {
        m_endDelay = d;
    }
    void setFill(String* s)
    {
        m_fill = s;
    }
    void setIterationStart(double d)
    {
        m_iterationStart = d;
    }
    void setIterations(double d)
    {
        m_iterations = d;
    }
    void setDuration(double d)
    {
        m_duration = d;
    }
    void setDirection(String* s)
    {
        m_direction = s;
    }
    void setEasing(String* s)
    {
        m_easing = s;
    }

    double delay() const
    {
        return m_delay;
    }

    double endDelay() const
    {
        return m_endDelay;
    }

    String* fill() const
    {
        return m_fill;
    }

    double iterationStart() const
    {
        return m_iterationStart;
    }

    double iterations() const
    {
        return m_iterations;
    }

    double duration() const
    {
        return m_duration;
    }

    String* direction() const
    {
        return m_direction;
    }

    String* easing() const
    {
        return m_easing;
    }

    double m_delay;
    String* m_direction;

    // TODO : handle this as unrestricted double or DOMString type
    double m_duration;
    String* m_easing;
    double m_endDelay;
    String* m_fill;
    double m_iterationStart;
    double m_iterations;
};

struct KeyframeEffectOptions : public EffectTiming {
    KeyframeEffectOptions()
        : EffectTiming()
    {
    }
};

struct KeyframeAnimationOptions : public KeyframeEffectOptions {
    KeyframeAnimationOptions()
        : KeyframeEffectOptions()
        , m_id(String::fromUTF8(""))
    {
    }

    void setId(String* s)
    {
        m_id = s;
    }

    String* id() const
    {
        return m_id;
    }

    String* m_id;
};

class Element;

class Animation : public EventTarget {
public:
    Animation();
    Animation(ExecutionContext* executionContext);
    ~Animation()
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isAnimation() const override;

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    // Element.animate() drives the animation through the CSS animation
    // machinery, so the effect is identified by its target and by the
    // generated animation name.
    void setEffectTarget(Element* target, String* animationName)
    {
        m_target = target;
        m_animationName = animationName;
    }

    void cancel();

    // Called by the executor when the underlying animation reached its end.
    void notifyFinished();

    // Called by the executor when it dropped the animation itself, e.g. when
    // the target went display:none.
    void notifyCanceled();

    // Called by the executor when a later animation replaced this one and its
    // filled values were removed.
    void notifyRemoved();

    ExecutionContext* m_executionContext;

private:
    void fireEvent(String* eventType);

    Element* m_target;
    String* m_animationName;
    bool m_isFinished;
    bool m_isCanceled;
};
} // namespace Starfish

#endif
