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

class StyleAnimationKeyframe : public gc {
public:
    static TimingFunction* defaultTimingFunction();

    StyleAnimationKeyframe()
        : m_keyframeName(0)
        , m_duration(CSSTime(0))
        , m_delay(CSSTime(0))
        , m_timingFunction(defaultTimingFunction())
    {
    }

    StyleAnimationKeyframe(const StyleAnimationKeyframe& keyframe)
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

class StyleAnimationData : public gc {
public:
    StyleAnimationData()
    {
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

    void clearAnimationNames()
    {
        m_nameList.clear();
    }

    void setAnimationName(String* name)
    {
        STARFISH_ASSERT(name != nullptr);
        m_nameList.push_back(name);
    }

    String* animationName(size_t index)
    {
        STARFISH_ASSERT(index <= m_nameList.size());
        return m_nameList[index];
    }

    size_t animationNameListSize()
    {
        return m_nameList.size();
    }

    void clearAnimationKeyframes()
    {
        m_keyframeList.clear();
    }

    void setAnimationKeyframe(StyleAnimationKeyframe* keyframe)
    {
        STARFISH_ASSERT(keyframe != nullptr);
        m_keyframeList.push_back(keyframe);
    }

    StyleAnimationKeyframe* animationKeyframe(size_t idx)
    {
        size_t size = m_keyframeList.size();
        if (size == 0) {
            return nullptr;
        }

        uint16_t p = idx % size;
        STARFISH_ASSERT(m_keyframeList[p] != nullptr);
        return m_keyframeList[p];
    }

    size_t animationKeyframeListSize()
    {
        return m_keyframeList.size();
    }

    void clearTimingFunctions()
    {
        m_timingFunctionList.clear();
    }

    void setTimingFunction(TimingFunction* f)
    {
        STARFISH_ASSERT(f != nullptr);
        m_timingFunctionList.push_back(f);
    }

    TimingFunction* timingFunction(size_t idx) const
    {
        size_t size = m_timingFunctionList.size();
        if (size == 0) {
            return StyleAnimationKeyframe::defaultTimingFunction();
        }
        uint16_t p = idx % size;
        STARFISH_ASSERT(m_timingFunctionList[p] != nullptr);
        return m_timingFunctionList[p];
    }

    size_t timingFunctionSize() const
    {
        return m_timingFunctionList.size();
    }

    void clearDurations()
    {
        m_durationList.clear();
    }

    void setDuration(CSSTime duration)
    {
        m_durationList.push_back(duration);
    }

    CSSTime duration(size_t idx) const
    {
        size_t size = m_durationList.size();
        if (size == 0) {
            return CSSTime(0.0);
        }
        uint16_t p = idx % size;
        return m_durationList[p];
    }

    size_t durationSize() const
    {
        return m_durationList.size();
    }

    enum Direction { NORMAL, REVERSE, ALTERNATE_NORMAL, ALTERNATE_REVERSE };
    enum FillMode { NONE, FORWARDS, BACKWARDS, BOTH, AUTO };
    enum PlayState { PAUSED, RUNNING };

private:
    GCVector<String*> m_nameList;
    GCVector<StyleAnimationKeyframe*> m_keyframeList;

    GCAtomicVector<CSSTime> m_durationList;
    GCAtomicVector<double> m_iterationCountList;
    GCAtomicVector<Direction> m_directionList;
    GCAtomicVector<FillMode> m_fileModeList;
    GCAtomicVector<PlayState> m_playStateList;
    GCVector<TimingFunction*> m_timingFunctionList;
};
}

#endif
