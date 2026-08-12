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
#include <utility>

#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"

namespace Starfish {

enum class AnimationDirectionValue ENSURE_ENUM_UNSIGNED;
enum class AnimationPlayStateValue ENSURE_ENUM_UNSIGNED;
enum class AnimationType ENSURE_ENUM_UNSIGNED;

class String;
class Element;
class Animation;
class ActiveAnimationTask;

class ActiveElementAnimation : public gc {
public:
    ActiveElementAnimation(String* name, Element* element,
                           AnimationType animationType, size_t index,
                           float iterationCount,
                           AnimationDirectionValue direction,
                           AnimationPlayStateValue playState)
        : m_hash(0)
        , m_name(name)
        , m_element(element)
        , m_animationType(animationType)
        , m_index(index)
        , m_duration(0)
        , m_delay(0)
        , m_iterationCount(iterationCount)
        , m_direction(direction)
        , m_playState(playState)
        , m_webAnimation(nullptr)
    {
        STARFISH_ASSERT(name != nullptr);
        STARFISH_ASSERT(element != nullptr);
    }

    // The script visible Animation object when this animation was started by
    // Element.animate(). Null for CSS and SVG animations.
    Animation* webAnimation() const
    {
        return m_webAnimation;
    }

    void setWebAnimation(Animation* animation)
    {
        m_webAnimation = animation;
    }

    String* name() const
    {
        return m_name;
    }

    Element* element() const
    {
        return m_element;
    }

    AnimationType animationType() const
    {
        return m_animationType;
    }

    size_t index() const
    {
        return m_index;
    }

    double duration() const
    {
        return m_duration;
    }

    double delay() const
    {
        return m_delay;
    }

    float iterationCount() const
    {
        return m_iterationCount;
    }

    AnimationDirectionValue direction() const
    {
        return m_direction;
    }

    AnimationPlayStateValue playState() const
    {
        return m_playState;
    }

    size_t hashValue() const;

    bool equals(const ActiveElementAnimation* src) const;

    bool isForwardDirection(ActiveAnimationTask* activeAnimationTask);

private:
    bool isOddIteration(ActiveAnimationTask* activeAnimationTask);

    mutable size_t m_hash;

    String* m_name;
    Element* m_element;
    AnimationType m_animationType;

    // FIXME: The following members are what each member of ActiveAnimationTask
    // has. I do not understand why this Class should have these values as
    // members in duplicates. so they should be removed from one of the two
    // classes, if possible.
    size_t m_index;
    double m_duration;
    double m_delay;
    float m_iterationCount;
    AnimationDirectionValue m_direction;
    AnimationPlayStateValue m_playState;
    Animation* m_webAnimation;
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

enum class SVGAnimationEventType {
    BeginEvent,
    RepeatEvent,
    EndEvent,
};

enum class KeyFramesAnimationEventType {
    AnimationStart,
    AnimationCancel,
    AnimationEnd,
};

class AnimationExecutor : public gc {
public:
    class ExecutionContext {
        friend class AnimationExecutor;
        STARFISH_MAKE_STACK_ALLOCATED();

    public:
        ExecutionContext(AnimationExecutor* executor, Element* element,
                         Optional<ComputedStyle*> fromStyle,
                         Optional<Frame*> oldFrame, ComputedStyle* toStyle,
                         uint64_t tick, ComputedStyleDamage& damage,
                         bool (&damagedKeys)[CSSStyleValuePair::KeyKindSize]);

        ~ExecutionContext();

        void begin();
        void end();

    private:
        void recomputeStyleDamageInAnimation();

        AnimationExecutor* m_executor;
        Element* m_element;
        Optional<ComputedStyle*> m_fromStyle;
        Optional<Frame*> m_oldFrame;
        ComputedStyle* m_toStyle;
        uint64_t m_tick;
        bool m_hasActiveTask;
        bool m_needsToCheckActiveExecutorInWebView;
        bool m_needsToRecomputeStylePropertyDamage;
        ComputedStyleDamage& m_damage;
        bool (&m_damagedKeys)[CSSStyleValuePair::KeyKindSize];
        std::vector<std::pair<CSSStyleValuePair::KeyKind, double>>
            m_canceledAnimationProgress;

        // std::pair<opacity, transform>
        std::pair<bool, bool> m_beforeRunningStates{ false, false };
        std::pair<bool, bool> m_afterRunningStates{ false, false };
    };

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

    bool hasActiveAnimationTask(String* name, Element* element,
                                AnimationType animationType,
                                CSSStyleValuePair::KeyKind p, size_t layer = 0);

    void removeActiveAnimationTaskIfNeeds(String* animationName,
                                          Element* element,
                                          AnimationType animationType,
                                          CSSStyleValuePair::KeyKind p,
                                          size_t layer = 0);
    void cancelActiveAnimationTaskIfNeeds(String* animationName,
                                          Element* element,
                                          AnimationType animationType,
                                          CSSStyleValuePair::KeyKind p,
                                          size_t layer = 0);

    void registerAnimation(ActiveAnimationTask* task, String* name,
                           size_t index, float iterationCount,
                           AnimationDirectionValue direction,
                           AnimationPlayStateValue playState,
                           AnimationType animationType);

    // Bind the script visible Animation object to the tasks that
    // Element.animate() has just registered under animationName.
    void attachWebAnimation(String* animationName, Element* element,
                            Animation* animation);

    // Drop every task of the Web Animation named animationName. Returns true
    // when at least one task was running.
    bool cancelWebAnimation(String* animationName, Element* element);

    // Drop every finished Web Animation of element whose properties are all
    // animated by the one named newAnimationName, so repeated calls to
    // Element.animate() do not pile up filled tasks.
    void removeReplacedWebAnimations(String* newAnimationName,
                                     Element* element);

    uint64_t transformOpacityAnimationRemainTime();

    void checkActiveTransitionsState(ExecutionContext& context);
    void addNewActiveTransitionIfNeeds(ExecutionContext& context);
    void executeActiveTransitionsStep(ExecutionContext& context);

    void checkActiveAnimationsState(ExecutionContext& context);
    void addNewActiveAnimationsIfNeeds(ExecutionContext& context);
    void executeActiveAnimationsStep(ExecutionContext& context);

    void fireKeyFramesAnimationEvent(KeyFramesAnimationEventType type,
                                     Element* element, String* animationName,
                                     double elapsedTime);

    void fireSVGAnimationEvents(
        const std::set<SVGAnimationElement*>& originAnimationElements,
        SVGAnimationEventType type);
    void fireSVGAnimationEvent(Element* element, SVGAnimationEventType type);

private:
    GCVector<ActiveAnimationTask*> m_activeTransitions;
    GCUnorderedMap<ActiveElementAnimation*, GCVector<ActiveAnimationTask*>>
        m_activeAnimations;
};
} // namespace Starfish

#endif
