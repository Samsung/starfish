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

#ifndef __StarFishStyleTransitionData__
#define __StarFishStyleTransitionData__

#include "core/animation/AnimationTimingFunction.h"

namespace StarFish {

class AnimationTimingFunction;

class StyleTransitionData : public gc {
public:
    StyleTransitionData()
        : m_property(TransitionPropertyValue::TransitionPropertyAllValue)
        , m_duration(0)
        , m_delay(0)
    {
    }

    ~StyleTransitionData()
    {
    }

    TransitionPropertyValue property()
    {
        return m_property;
    }

    void setProperty(TransitionPropertyValue property)
    {
        m_property = property;
    }

    CSSTime duration()
    {
        return m_duration;
    }

    void setDuration(CSSTime duration)
    {
        m_duration = duration;
    }

    CSSTime delay()
    {
        return m_delay;
    }

    void setDelay(CSSTime delay)
    {
        m_delay = delay;
    }

    AnimationTimingFunction* timingFunction()
    {
        return m_timingFunction;
    }

    void setTimingFunction(AnimationTimingFunction* f)
    {
        m_timingFunction = f;
    }

private:
    friend inline bool operator==(const StyleTransitionData& a,
                                  const StyleTransitionData& b);
    friend inline bool operator!=(const StyleTransitionData& a,
                                  const StyleTransitionData& b);

    TransitionPropertyValue m_property;
    AnimationTimingFunction* m_timingFunction;
    CSSTime m_duration;
    CSSTime m_delay;
};

bool operator==(const StyleTransitionData& a, const StyleTransitionData& b)
{
    if (a.m_property != b.m_property) {
        return false;
    }

    if (a.m_duration != b.m_duration) {
        return false;
    }

    if (a.m_delay != b.m_delay) {
        return false;
    }

    if (*a.m_timingFunction != *b.m_timingFunction) {
        return false;
    }

    return true;
}

bool operator!=(const StyleTransitionData& a, const StyleTransitionData& b)
{
    return !operator==(a, b);
}
}

#endif
