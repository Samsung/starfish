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

#ifndef __StarfishStyleAnimationData__
#define __StarfishStyleAnimationData__

namespace Starfish {

class TimingFunction;

class AnimationKeyframe : public gc {
public:
    static TimingFunction* defaultTimingFunction();

    AnimationKeyframe()
        : m_keyframeName(0)
        , m_duration(CSSTime(0))
        , m_delay(CSSTime(0))
        , m_timingFunction(defaultTimingFunction())
    {
    }

    AnimationKeyframe(const AnimationKeyframe& keyframe)
        : m_keyframeName(keyframe.keyframeName())
        , m_duration(keyframe.duration())
        , m_delay(keyframe.delay())
        , m_timingFunction(keyframe.timingFunction())
    {
        m_properties.assign(keyframe.properties().begin(),
                            keyframe.properties().end());
        m_keyKinds.assign(keyframe.keyKinds().begin(),
                          keyframe.keyKinds().end());
    }

    double keyframeName() const
    {
        return m_keyframeName;
    }

    void setKeyframeName(double name)
    {
        m_keyframeName = name;
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

    TimingFunction* timingFunction() const
    {
        return m_timingFunction;
    }

    void setTimingFunction(TimingFunction* f)
    {
        STARFISH_ASSERT(f != nullptr);
        m_timingFunction = f;
    }

    const GCVector<CSSStyleValuePair>& properties() const
    {
        return m_properties;
    }

    const GCVector<CSSStyleValuePair::KeyKind>& keyKinds() const
    {
        return m_keyKinds;
    }

    void setProperty(CSSStyleValuePair::KeyKind keyKind,
                     CSSStyleValuePair property)
    {
        size_t idx = keyKindIndex(keyKind);
        if (idx != SIZE_MAX) {
            m_properties[idx] = property;
        } else {
            m_properties.push_back(property);
            m_keyKinds.push_back(keyKind);
        }
    }

    size_t propertySize() const
    {
        STARFISH_ASSERT(m_properties.size() == m_keyKinds.size());
        return m_properties.size();
    }

    size_t keyKindIndex(const CSSStyleValuePair::KeyKind& keyKind)
    {
        for (size_t i = 0; i < m_keyKinds.size(); i++) {
            if (keyKind == m_keyKinds[i]) {
                return i;
            }
        }
        return SIZE_MAX;
    }

private:
    double m_keyframeName;
    CSSTime m_duration;
    CSSTime m_delay;
    TimingFunction* m_timingFunction;
    GCVector<CSSStyleValuePair> m_properties;
    GCVector<CSSStyleValuePair::KeyKind> m_keyKinds;
};

class AnimationKeyframes : public gc {
public:
    AnimationKeyframes()
        : m_name(String::emptyString)
        , m_duration(0)
        , m_delay(0)
        , m_timingFunction(AnimationKeyframe::defaultTimingFunction())
    {
    }

    void setName(String* name)
    {
        STARFISH_ASSERT(name != nullptr);
        m_name = name;
    }

    String* name()
    {
        return m_name;
    }

    void setTimingFunction(TimingFunction* f)
    {
        STARFISH_ASSERT(f != nullptr);
        m_timingFunction = f;
    }

    TimingFunction* timingFunction() const
    {
        return m_timingFunction;
    }

    void setDuration(CSSTime t)
    {
        m_duration = t;
    }

    CSSTime duration() const
    {
        return m_duration;
    }

    void setDelay(CSSTime t)
    {
        m_delay = t;
    }

    CSSTime delay() const
    {
        return m_delay;
    }

    size_t keyframeListSize()
    {
        return m_keyframeList.size();
    }

    GCVector<AnimationKeyframe*>& keyframeList()
    {
        return m_keyframeList;
    }

    AnimationKeyframe* keyframe(size_t index)
    {
        STARFISH_ASSERT(m_keyframeList.size() > index);
        return m_keyframeList[index];
    }

private:
    String* m_name;
    CSSTime m_duration;
    CSSTime m_delay;
    TimingFunction* m_timingFunction;
    GCVector<AnimationKeyframe*> m_keyframeList;
};

class StyleAnimationData : public gc {
public:
    StyleAnimationData()
        : m_nameSize(0)
        , m_durationSize(0)
        , m_timingFunctionSize(0)
        , m_delaySize(0)
    {
    }

    size_t allKeyframeListSize()
    {
        size_t size = 0;
        for (size_t i = 0; i < keyframesSize(); i++) {
            size += m_keyframes[i].keyframeListSize();
        }
        return size;
    }

    size_t keyframesSize()
    {
        return m_keyframes.size();
    }

    AnimationKeyframes& keyframes(size_t index)
    {
        STARFISH_ASSERT(m_keyframes.size() > index);
        return m_keyframes[index];
    }

    void resizeIfNeeds(size_t index, size_t& currentSize)
    {
        if (m_keyframes.size() <= index) {
            m_keyframes.resize(index + 1);
        }
        if (currentSize <= index) {
            currentSize = index + 1;
        }
    }

    bool operator==(const StyleAnimationData& b) const
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }

    bool operator!=(const StyleAnimationData& b) const
    {
        return !operator==(b);
    }

    // animation-name /////////////////////////////
    void clearAnimationNames()
    {
        m_nameSize = 0;
    }

    void setAnimationName(String* name, size_t index)
    {
        STARFISH_ASSERT(name != nullptr);
        resizeIfNeeds(index, m_nameSize);
        m_keyframes[index].setName(name);
    }

    String* animationName(size_t index)
    {
        STARFISH_ASSERT(m_nameSize <= m_keyframes.size());
        STARFISH_ASSERT(index < m_nameSize);
        return m_keyframes[index].name();
    }

    size_t animationNameSize()
    {
        return m_nameSize;
    }

    // animation-timing-function /////////////////////////////
    void clearTimingFunctions()
    {
        m_timingFunctionSize = 0;
    }

    void setTimingFunction(TimingFunction* f, size_t index)
    {
        STARFISH_ASSERT(f != nullptr);
        resizeIfNeeds(index, m_timingFunctionSize);
        m_keyframes[index].setTimingFunction(f);
    }

    TimingFunction* timingFunction(size_t index) const
    {
        STARFISH_ASSERT(m_timingFunctionSize <= m_keyframes.size());
        if (m_timingFunctionSize == 0) {
            return AnimationKeyframe::defaultTimingFunction();
        }
        uint16_t p = index % m_timingFunctionSize;
        STARFISH_ASSERT(m_keyframes[p].timingFunction() != nullptr);
        return m_keyframes[p].timingFunction();
    }

    size_t timingFunctionSize() const
    {
        return m_timingFunctionSize;
    }

    // animation-duration /////////////////////////////
    void clearDurations()
    {
        m_durationSize = 0;
    }

    void setDuration(CSSTime duration, size_t index)
    {
        resizeIfNeeds(index, m_durationSize);
        m_keyframes[index].setDuration(duration);
    }

    CSSTime duration(size_t index) const
    {
        STARFISH_ASSERT(m_durationSize <= m_keyframes.size());
        if (m_durationSize == 0) {
            return CSSTime(0);
        }
        uint16_t p = index % m_durationSize;
        return m_keyframes[p].duration();
    }

    size_t durationSize() const
    {
        return m_durationSize;
    }

    // animation-delay /////////////////////////////
    void clearDelays()
    {
        m_delaySize = 0;
    }

    void setDelay(CSSTime delay, size_t index)
    {
        resizeIfNeeds(index, m_delaySize);
        m_keyframes[index].setDelay(delay);
    }

    CSSTime delay(size_t index) const
    {
        STARFISH_ASSERT(m_delaySize <= m_keyframes.size());
        if (m_delaySize == 0) {
            return CSSTime(0);
        }
        uint16_t p = index % m_delaySize;
        return m_keyframes[p].delay();
    }

    size_t delaySize() const
    {
        return m_delaySize;
    }

    enum Direction { NORMAL, REVERSE, ALTERNATE_NORMAL, ALTERNATE_REVERSE };
    enum FillMode { NONE, FORWARDS, BACKWARDS, BOTH, AUTO };
    enum PlayState { PAUSED, RUNNING };

private:
    GCVector<AnimationKeyframes> m_keyframes;
    size_t m_nameSize;
    size_t m_durationSize;
    size_t m_timingFunctionSize;
    size_t m_delaySize;
};
}

#endif
