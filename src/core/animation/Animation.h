/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

    double delay()
    {
        return m_delay;
    }
    double endDelay()
    {
        return m_endDelay;
    }
    String* fill()
    {
        return m_fill;
    }
    double iterationStart()
    {
        return m_iterationStart;
    }
    double iterations()
    {
        return m_iterations;
    }
    double duration()
    {
        return m_duration;
    }
    String* direction()
    {
        return m_direction;
    }
    String* easing()
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

    String* id()
    {
        return m_id;
    }

    String* m_id;
};

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

    ExecutionContext* m_executionContext;
};
}

#endif
