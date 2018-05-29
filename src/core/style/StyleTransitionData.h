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
class StyleTransitionData;

class StyleTransitionLayer : public gc {
    friend StyleTransitionData;

public:
    StyleTransitionLayer();

    ~StyleTransitionLayer()
    {
    }

private:
    CSSStyleValuePair::KeyKind property() const
    {
        return m_property;
    }

    void setProperty(CSSStyleValuePair::KeyKind property)
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
    CSSStyleValuePair::KeyKind m_property;
    AnimationTimingFunction* m_timingFunction;
    CSSTime m_duration;
    CSSTime m_delay;
};

class StyleTransitionData : public gc {
public:
    StyleTransitionData()
        : m_propertySize(0)
        , m_durationSize(0)
        , m_timingFunctionSize(0)
        , m_delaySize(0)
    {
    }
    static AnimationTimingFunction* defaultTimingFunction();

    size_t size() const
    {
        return m_propertySize;
    }

    void resizeIfNeeds(size_t index, size_t& currentSize)
    {
        if (m_layers.size() <= index) {
            m_layers.resize(index + 1);
        }
        if (currentSize <= index) {
            currentSize = index + 1;
        }
    }

    void shrinkProperties(size_t newsize)
    {
        m_propertySize = newsize;
        // NOTE May need shrinkSizeIfNeeds() here
    }

    void shrinkDurations(size_t newsize)
    {
        m_durationSize = newsize;
        // NOTE May need shrinkSizeIfNeeds() here
    }

    void shrinkDelays(size_t newsize)
    {
        m_delaySize = newsize;
        // NOTE May need shrinkSizeIfNeeds() here
    }

    void shrinkTimingFunctions(size_t newsize)
    {
        STARFISH_ASSERT(m_timingFunctionSize <= m_layers.size());
        // NOTE Remove references to release AnimationTimingFunction (GC)
        for (size_t i = newsize; i < m_timingFunctionSize; i++) {
            m_layers[i].setTimingFunction(nullptr);
        }
        m_timingFunctionSize = newsize;
        // NOTE May need shrinkSizeIfNeeds() here
    }

    CSSStyleValuePair::KeyKind property(size_t layer) const
    {
        STARFISH_ASSERT(m_propertySize <= m_layers.size());
        STARFISH_ASSERT(layer < m_propertySize);
        return m_layers[layer].property();
    }

    CSSTime duration(size_t layer) const
    {
        STARFISH_ASSERT(m_durationSize <= m_layers.size());
        if (m_durationSize == 0) {
            return 0;
        }
        uint16_t p = layer % m_durationSize;
        return m_layers[p].duration();
    }

    CSSTime delay(size_t layer) const
    {
        STARFISH_ASSERT(m_delaySize <= m_layers.size());
        if (m_delaySize == 0) {
            return 0;
        }
        uint16_t p = layer % m_delaySize;
        return m_layers[p].delay();
    }

    AnimationTimingFunction* timingFunction(size_t layer) const
    {
        STARFISH_ASSERT(m_timingFunctionSize <= m_layers.size());
        if (m_timingFunctionSize == 0) {
            return defaultTimingFunction();
        }
        uint16_t p = layer % m_timingFunctionSize;
        STARFISH_ASSERT(m_layers[p].timingFunction());
        return m_layers[p].timingFunction();
    }

    void setProperty(CSSStyleValuePair::KeyKind property, size_t layer)
    {
        resizeIfNeeds(layer, m_propertySize);
        m_layers[layer].setProperty(property);
    }

    void setDuration(CSSTime duration, size_t layer)
    {
        resizeIfNeeds(layer, m_durationSize);
        m_layers[layer].setDuration(duration);
    }

    void setDelay(CSSTime delay, size_t layer)
    {
        resizeIfNeeds(layer, m_delaySize);
        m_layers[layer].setDelay(delay);
    }

    void setTimingFunction(AnimationTimingFunction* f, size_t layer)
    {
        STARFISH_ASSERT(f);
        resizeIfNeeds(layer, m_timingFunctionSize);
        m_layers[layer].setTimingFunction(f);
    }

    bool operator==(const StyleTransitionData& b) const;
    bool operator!=(const StyleTransitionData& b) const
    {
        return !operator==(b);
    }

private:
    GCVector<StyleTransitionLayer> m_layers;
    size_t m_propertySize;
    size_t m_durationSize;
    size_t m_timingFunctionSize;
    size_t m_delaySize;
};
}

#endif
