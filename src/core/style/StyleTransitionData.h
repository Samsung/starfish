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

namespace StarFish {

class AnimationTimingFunction;

class StyleTransitionLayer : public gc {
public:
    StyleTransitionLayer();

    ~StyleTransitionLayer()
    {
    }

    TransitionPropertyValue property() const
    {
        return m_property;
    }

    void setProperty(TransitionPropertyValue property)
    {
        m_property = property;
    }

    CSSTime duration() const
    {
        return m_duration;
    }

    void setDuration(CSSTime duration)
    {
        m_duration = duration;
    }

    CSSTime delay() const
    {
        return m_delay;
    }

    void setDelay(CSSTime delay)
    {
        m_delay = delay;
    }

    AnimationTimingFunction* timingFunction() const
    {
        return m_timingFunction;
    }

    void setTimingFunction(AnimationTimingFunction* f)
    {
        m_timingFunction = f;
    }

    bool operator==(const StyleTransitionLayer& b) const;
    bool operator!=(const StyleTransitionLayer& b) const;

private:
    TransitionPropertyValue m_property;
    AnimationTimingFunction* m_timingFunction;
    CSSTime m_duration;
    CSSTime m_delay;
};

class StyleTransitionData : public GCVector<StyleTransitionLayer> {
public:
    static AnimationTimingFunction* defaultTimingFunction();

    TransitionPropertyValue property(size_t layer = 0) const
    {
        if (size() <= layer) {
            return TransitionPropertyAllValue;
        }
        return at(layer).property();
    }

    void setProperty(TransitionPropertyValue property, size_t layer = 0)
    {
        if (size() <= layer) {
            resize(layer + 1);
        }
        at(layer).setProperty(property);
    }

    CSSTime duration(size_t layer = 0) const
    {
        if (size() <= layer) {
            return 0;
        }
        return at(layer).duration();
    }

    void setDuration(CSSTime duration, size_t layer = 0)
    {
        if (size() <= layer) {
            resize(layer + 1);
        }
        at(layer).setDuration(duration);
    }

    CSSTime delay(size_t layer = 0) const
    {
        if (size() <= layer) {
            return 0;
        }
        return at(layer).delay();
    }

    void setDelay(CSSTime delay, size_t layer = 0)
    {
        if (size() <= layer) {
            resize(layer + 1);
        }
        at(layer).setDelay(delay);
    }

    AnimationTimingFunction* timingFunction(size_t layer = 0) const;

    void setTimingFunction(AnimationTimingFunction* f, size_t layer = 0)
    {
        if (size() <= layer) {
            resize(layer + 1);
        }
        at(layer).setTimingFunction(f);
    }

    bool operator==(const StyleTransitionData& b) const
    {
        size_t len = size();
        for (size_t i = 0; i < len; i++) {
            if (at(i) != b.at(i)) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const StyleTransitionData& b) const
    {
        return !operator==(b);
    }
};
}

#endif
