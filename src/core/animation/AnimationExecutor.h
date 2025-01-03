/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAnimationExecutor__
#define __StarfishAnimationExecutor__

#include <cstddef>
#include <functional>

#include "core/style/Style.h"

namespace Starfish {

enum class AnimationDirectionValue ENSURE_ENUM_UNSIGNED;
enum class AnimationPlayStateValue ENSURE_ENUM_UNSIGNED;

class String;
class Element;
class ActiveAnimationTask;

struct ActiveElementAnimation : public gc {
    String* m_name;
    Element* m_element;
    size_t m_index;
    double m_duration;
    double m_delay;
    float m_iterationCount;
    AnimationDirectionValue m_direction;
    AnimationPlayStateValue m_playState;

    ActiveElementAnimation(String* name, Element* element, size_t index,
                           float iterationCount,
                           AnimationDirectionValue direction,
                           AnimationPlayStateValue playState)
        : m_name(name)
        , m_element(element)
        , m_index(index)
        , m_duration(0)
        , m_delay(0)
        , m_iterationCount(iterationCount)
        , m_direction(direction)
        , m_playState(playState)
        , m_hash(0)
    {
        STARFISH_ASSERT(name != nullptr);
        STARFISH_ASSERT(element != nullptr);
    }

    size_t hashValue() const;
    bool equals(const ActiveElementAnimation* src) const;

private:
    mutable size_t m_hash;
};
} // namespace Starfish

namespace std {
template <>
struct hash<Starfish::ActiveElementAnimation*> {
    std::size_t operator()(const Starfish::ActiveElementAnimation* value) const
    {
        return value->hashValue();
    }
};

template <>
struct equal_to<Starfish::ActiveElementAnimation*> {
    bool operator()(const Starfish::ActiveElementAnimation* lhs,
                    const Starfish::ActiveElementAnimation* rhs) const
    {
        return lhs->equals(rhs);
    }
};

} // namespace std

namespace Starfish {

class AnimationExecutor : public gc {
public:
    AnimationExecutor()
    {
    }

    GCVector<ActiveAnimationTask*>& activeTransitions()
    {
        return m_activeTransitions;
    }

    GCUnorderedMap<ActiveElementAnimation*, GCVector<ActiveAnimationTask*>>&
    activeAnimations()
    {
        return m_activeAnimations;
    }

    void iterateAnimationTasks(void (*fn)(ActiveAnimationTask*, void*), void*);

    void dispose();

    bool hasActiveTransition(Element* element, CSSStyleValuePair::KeyKind p);

    void registerTransition(ActiveAnimationTask* task);

    void removeActiveAnimationTaskIfNeeds(Element* element,
                                          CSSStyleValuePair::KeyKind p,
                                          size_t layer = 0);

    void registerAnimation(ActiveAnimationTask* task, String* name,
                           size_t index, float iterationCount,
                           AnimationDirectionValue direction,
                           AnimationPlayStateValue playState,
                           bool isCSSAnimationTask);

    uint64_t transformOpacityAnimationRemainTime();

    void fireAnimationStartEvent(Element* element, String* name, double delay);
    void fireAnimationEndEvent(Element* element, String* name,
                               double elapsedTime);
    void fireAnimationCancelEvent(Element* element, String* name,
                                  double elapsedTime);

    void fireSVGAnimateBeginEvent(Element* element);

private:
    GCVector<ActiveAnimationTask*> m_activeTransitions;
    GCUnorderedMap<ActiveElementAnimation*, GCVector<ActiveAnimationTask*>>
        m_activeAnimations;
};
} // namespace Starfish

#endif
