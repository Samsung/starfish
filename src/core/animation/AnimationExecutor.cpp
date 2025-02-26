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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/animation/AnimationExecutor.h"
#include "core/animation/AnimationTask.h"
#include "core/animation/AnimationApplier.h"
#include "core/dom/AnimationEvent.h"
#include "core/dom/Element.h"
#include "core/dom/svg/SVGAnimationElement.h"

namespace Starfish {

size_t ActiveElementAnimation::hashValue() const
{
    if (m_hash == 0) {
        hash_combine(m_hash, m_name->hashValue());
        hash_combine(m_hash, (size_t)m_element);
        hash_combine(m_hash, static_cast<size_t>(m_animationType));
    }
    return m_hash;
}

bool ActiveElementAnimation::equals(const ActiveElementAnimation* other) const
{
    STARFISH_ASSERT(other != nullptr);
    return (m_name->equals(other->m_name) == true) &&
           (m_element == other->m_element) &&
           (m_animationType == other->m_animationType);
}

bool ActiveElementAnimation::isOddIteration(
    ActiveAnimationTask* activeAnimationTask)
{
    float iterationCount = this->iterationCount();
    float iterationStart = activeAnimationTask->iterationStart();

    bool ret = false;
    if (!std::isinf(iterationCount)) {
        ret = std::fmod(iterationCount - iterationStart + 1, 2) >= 1;
    } else {
        ret = std::fmod(iterationStart, 2) >= 1;
    }
    return ret;
}

bool ActiveElementAnimation::isForwardDirection(
    ActiveAnimationTask* activeAnimationTask)
{
    bool isOdd = isOddIteration(activeAnimationTask);
    AnimationDirectionValue dir = this->direction();
    bool ret = (dir == AnimationDirectionValue::Normal) ||
               (dir == AnimationDirectionValue::Alternate && isOdd) ||
               (dir == AnimationDirectionValue::AlternateReverse && !isOdd);
    return ret;
}

void AnimationExecutor::iterateAnimationTasks(void (*fn)(ActiveAnimationTask*,
                                                         void*),
                                              void* data)
{
    for (size_t i = 0; i < m_activeTransitions.size(); i++) {
        fn(m_activeTransitions[i], data);
    }

    for (auto animations = m_activeAnimations.begin();
         animations != m_activeAnimations.end(); animations++) {
        const auto& v = (*animations).second;
        for (size_t i = 0; i < v.size(); i++) {
            fn(v[i], data);
        }
    }
}

void AnimationExecutor::dispose()
{
    if (m_activeTransitions.size() > 0) {
        m_activeTransitions.clear();
    }
    for (auto iter = m_activeAnimations.begin();
         iter != m_activeAnimations.end(); iter++) {
        iter.value().clear();
    }
    m_activeAnimations.clear();
}

bool AnimationExecutor::hasActiveTransition(Element* element,
                                            CSSStyleValuePair::KeyKind p)
{
    STARFISH_ASSERT(element != nullptr);
    for (size_t i = 0; i < m_activeTransitions.size(); i++) {
        if (m_activeTransitions[i]->targetElement() == element &&
            m_activeTransitions[i]->property() == p) {
            return true;
        }
    }
    return false;
}

void AnimationExecutor::registerTransition(ActiveAnimationTask* task)
{
    m_activeTransitions.push_back(task);
    task->attachToElement();
    task->fireTransitionStartEvent();
}

void AnimationExecutor::removeActiveAnimationTaskIfNeeds(
    Element* element, AnimationType animationType, CSSStyleValuePair::KeyKind p,
    size_t layer)
{
    STARFISH_ASSERT(element != nullptr);

    for (auto animations = m_activeAnimations.begin();
         animations != m_activeAnimations.end();) {
        if ((*animations).second.size() == 0) {
            animations = m_activeAnimations.erase(animations);
        } else {
            for (auto task = (*animations).second.begin();
                 task != (*animations).second.end();) {
                if ((*task)->targetElement() == element &&
                    (*task)->animationType() == animationType &&
                    (*task)->property() == p &&
                    (*task)->layerIndex() == layer) {
                    task = animations.value().erase(task);
                } else {
                    task++;
                }
            }
            animations++;
        }
    }
}

void AnimationExecutor::registerAnimation(ActiveAnimationTask* task,
                                          String* name, size_t index,
                                          float iterationCount,
                                          AnimationDirectionValue direction,
                                          AnimationPlayStateValue playState,
                                          AnimationType animationType)
{
    task->attachToElement();
    task->setIterationStart(iterationCount);
    task->setIsRunning(playState == AnimationPlayStateValue::Running);

    ActiveElementAnimation* key =
        new ActiveElementAnimation(name, task->targetElement(), animationType,
                                   index, iterationCount, direction, playState);
    auto iter = m_activeAnimations.find(key);
    if (iter == m_activeAnimations.end()) {
        GCVector<ActiveAnimationTask*> v;
        v.push_back(task);
        m_activeAnimations.insert(std::make_pair(key, v));
    } else {
        // Because all of tasks with same property are already removed in
        // removeActiveAnimationTaskIfNeeds(), just add the task to vector.
        iter.value().push_back(task);
    }
}

uint64_t AnimationExecutor::transformOpacityAnimationRemainTime()
{
    uint64_t result = 0;
    uint64_t tick = tickCount();

    struct Data {
        uint64_t* result;
        uint64_t* tick;
    } d;
    d.result = &result;
    d.tick = &tick;

    iterateAnimationTasks(
        [](ActiveAnimationTask* task, void* data) {
            Data* d = (Data*)data;
            if (task->property() == CSSStyleValuePair::KeyKind::Opacity ||
                task->property() == CSSStyleValuePair::KeyKind::Transform) {
                *d->result = std::max(*d->result, task->remainTime(*d->tick) +
                                                      17); // add 1/60s
            }
        },
        &d);

    return result;
}

void AnimationExecutor::checkActiveTransitionsState(ExecutionContext& context)
{
    for (size_t i = 0; i < m_activeTransitions.size(); i++) {
        STARFISH_ASSERT(m_activeTransitions[i]->isTransition());
        if (m_activeTransitions[i]->targetElement() != context.element) {
            continue;
        }

        bool shouldRemove = false;
        bool isCancel = true;
        // time is up
        if (m_activeTransitions[i]->fraction(context.tick) >= 1) {
            shouldRemove = true;
            isCancel = false;
        }

        // element invisible
        if (!shouldRemove &&
            context.toStyle->display() == DisplayValue::NoneDisplayValue) {
            shouldRemove = true;
        }

        // transition targetToValue changed
        if (!shouldRemove &&
            !m_activeTransitions[i]->taskCanContinue(context.toStyle)) {
            shouldRemove = true;
        }

        // transition property gone || other properties changed
        if (!shouldRemove) {
            StyleTransitionData* data = context.toStyle->transition();
            if (data == nullptr) {
                shouldRemove = true;
            } else {
                bool found = false;
                for (size_t j = 0; j < data->size(); j++) {
                    if (data->property(j) == CSSStyleValuePair::KeyKind::All) {
                        found = true;
                        break;
                    }
                    if (m_activeTransitions[i]->isKindOfTransitionProperty(
                            data->property(j))) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    shouldRemove = true;
                }
            }
        }

        if (shouldRemove) {
            if (!isCancel) {
                context.damagedKeys[m_activeTransitions[i]->property()] = false;
                m_activeTransitions[i]->fireTransitionEndEvent();
            } else {
                auto key = m_activeTransitions[i]->property();
                double progress =
                    m_activeTransitions[i]->fraction(context.tick);
                context.canceledAnimationProgress.push_back(
                    std::make_pair(key, progress));
                m_activeTransitions[i]->fireTransitionCancelEvent();
            }
            m_activeTransitions[i]->detachFromElement();
            m_activeTransitions.erase(i);
            context.needsToRecomputeStylePropertyDamage = true;
            context.needsToCheckActiveExecutorInWebView = true;

            if (!m_activeTransitions.size()) {
                break;
            }
            i--;
        } else {
            context.hasActiveTask = true;
        }
    }
}

void AnimationExecutor::addNewActiveTransitionIfNeeds(ExecutionContext& context)
{
    if (!context.fromStyle.hasValue() ||
        context.fromStyle->display() == DisplayValue::NoneDisplayValue) {
        return;
    }

    if (context.toStyle->display() == DisplayValue::NoneDisplayValue ||
        context.toStyle->transitionLayerSize() == 0) {
        return;
    }

    if (context.damage == ComputedStyleDamage::ComputedStyleDamageNone) {
        return;
    }

    if (applyTransitionIfNeeds(context.element, context.fromStyle.getValue(),
                               context.oldFrame, context.toStyle,
                               context.damagedKeys,
                               context.canceledAnimationProgress)) {
        context.hasActiveTask = true;
        context.needsToCheckActiveExecutorInWebView = true;
    }
}

void AnimationExecutor::executeActiveTransitionsStep(ExecutionContext& context)
{
    if (!context.hasActiveTask) {
        return;
    }

    for (size_t i = 0; i < m_activeTransitions.size(); i++) {
        if (m_activeTransitions[i]->targetElement() == context.element) {
            m_activeTransitions[i]->step(context.tick, context.toStyle);
        }
    }
    context.needsToRecomputeStylePropertyDamage = true;
}

void AnimationExecutor::checkActiveAnimationsState(ExecutionContext& context)
{
    double cancelTick = 0.0;
    double endTick = 0.0;
    std::vector<ActiveAnimationTask*> expiredAnimationTasks;

    auto iter = m_activeAnimations.begin();
    while (iter != m_activeAnimations.end()) {
        ActiveElementAnimation* activeElementAnimation = iter.key();
        GCVector<ActiveAnimationTask*>& animationTasks = iter.value();
        if (activeElementAnimation->element() != context.element) {
            iter++;
            continue;
        }

        bool needsToFireAnimationEndEvent = false;
        bool needsToFireAnimationCancelEvent = false;
        float iterationCount = activeElementAnimation->iterationCount();
        for (size_t i = 0; i < animationTasks.size(); i++) {
            ActiveAnimationTask* task = animationTasks[i];
            STARFISH_ASSERT(!task->isTransition());

            if (task->targetElement() != context.element) {
                continue;
            }

            bool shouldRemove = false;
            bool isCancel = true;

            task->setIsForward(
                activeElementAnimation->isForwardDirection(task));

            if (task->fraction(context.tick) >= 1) {
                if (std::isinf(iterationCount)) {
                    float f = task->iterationStart() == 1 ? 0 : 1;
                    task->setIterationStart(f);
                } else {
                    float f = task->iterationStart() - 1;
                    task->setIterationStart(f);
                    if (task->iterationStart() < 1) {
                        // time is up
                        shouldRemove = true;
                        isCancel = false;
                        task->setIterationStart(iterationCount);
                    }
                }
            }

            // element invisible
            if (!shouldRemove &&
                context.toStyle->display() == DisplayValue::NoneDisplayValue) {
                shouldRemove = true;
            }

            // animation property gone || other properties changed
            if (!shouldRemove &&
                task->animationType() == AnimationType::KeyFramesAnimation) {
                if (context.toStyle->animation()) {
                    StyleAnimationData* styleAnimationData =
                        context.toStyle->animation();
                    for (size_t n = 0;
                         n < styleAnimationData->animationKeyframesListSize();
                         n++) {
                        if (styleAnimationData->animationName(n)->equals(
                                "none")) {
                            // animation name is gone.
                            shouldRemove = true;
                        }

                        if (!activeElementAnimation->name()->equals(
                                styleAnimationData->animationName(n))) {
                            continue;
                        }

                        bool found = false;
                        AnimationKeyframes& animationKeyframes =
                            styleAnimationData->animationKeyframes(n);
                        if (animationKeyframes.animationKeyframeListSize() >
                            0) {
                            AnimationKeyframe* animationKeyframe =
                                animationKeyframes.animationKeyframe(0);
                            for (auto& keyKind :
                                 animationKeyframe->keyKinds()) {
                                if (task->isKindOfTransitionProperty(keyKind)) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (!found) {
                            shouldRemove = true;
                        }
                    }
                } else {
                    shouldRemove = true;
                }
            }

            if (shouldRemove) {
                if (!isCancel) {
                    endTick = task->duration() / 1000.0;
                    needsToFireAnimationEndEvent = true;
                } else {
                    auto key = task->property();
                    double progress = task->fraction(context.tick);
                    context.canceledAnimationProgress.push_back(
                        std::make_pair(key, progress));

                    cancelTick = task->duration() * progress / 1000.0;
                    needsToFireAnimationCancelEvent = true;
                }
                // FIXME
                // TODO: What is FIXME for?
                task->detachFromElement();

                if (task->fillMode() != AnimationFillModeValue::Forwards) {
                    animationTasks.erase(i);
                    i--;
                } else {
                    context.hasActiveTask = true;
                    // if already in fill-mode, we should not fire end event
                    if (task->isInForwardsFillMode()) {
                        needsToFireAnimationEndEvent = false;
                    }
                    task->markInForwardsFillMode();
                }

                if (needsToFireAnimationEndEvent &&
                    activeElementAnimation->animationType() ==
                        AnimationType::SVGAnimation) {
                    expiredAnimationTasks.push_back(task);
                }
                context.needsToRecomputeStylePropertyDamage = true;
                context.needsToCheckActiveExecutorInWebView = true;
            } else {
                context.hasActiveTask = true;
            }
        }

        if (activeElementAnimation->animationType() ==
            AnimationType::SVGAnimation) {
            for (auto& expired : expiredAnimationTasks) {
                STARFISH_ASSERT(expired->originAnimationElement().hasValue());
                SVGAnimationElement* target =
                    expired->originAnimationElement().getValue();
                fireSVGAnimateEndEvent(target);
            }
        } else {
            if (needsToFireAnimationCancelEvent) {
                fireAnimationCancelEvent(context.element,
                                         activeElementAnimation->name(),
                                         cancelTick);
            } else if (needsToFireAnimationEndEvent) {
                fireAnimationEndEvent(context.element,
                                      activeElementAnimation->name(), endTick);
            }
        }

        if (animationTasks.empty()) {
            // if animationTasks is empty, remove it from activeAnimations in
            // executor. if both activeTransitions and activeAnimations in the
            // executor are empty, it is removed from the webview's active
            // animation executor list.
            iter = m_activeAnimations.erase(iter);
        } else {
            iter++;
        }
    }
}

void AnimationExecutor::addNewActiveAnimationsIfNeeds(ExecutionContext& context)
{
    if (context.toStyle->display() == DisplayValue::NoneDisplayValue) {
        return;
    }

    if (context.damage == ComputedStyleDamage::ComputedStyleDamageNone) {
        return;
    }

    if (context.element->didPrepareAnimation()) {
        return;
    }

    StyleAnimationData* styleAnimationData = context.toStyle->animation();
    if (!styleAnimationData ||
        !styleAnimationData->totalAnimationKeyframesListSize()) {
        return;
    }

    AnimationApplier animationApplier(context.element,
                                      AnimationType::KeyFramesAnimation,
                                      context.toStyle, nullptr);
    if (animationApplier.apply()) {
        context.hasActiveTask = true;
        context.needsToCheckActiveExecutorInWebView = true;
    }
}

void AnimationExecutor::executeActiveAnimationsStep(ExecutionContext& context)
{
    if (!context.hasActiveTask) {
        return;
    }

    for (auto& pair : m_activeAnimations) {
        if (pair.first->element() != context.element) {
            continue;
        }
        auto& activeAnimationTasks = pair.second;
        for (auto* task : activeAnimationTasks) {
            if (task->targetElement() == context.element) {
                task->step(context.tick, context.toStyle);
            }
        }
    }
    context.needsToRecomputeStylePropertyDamage = true;
}

void AnimationExecutor::fireAnimationStartEvent(Element* element, String* name,
                                                double delay)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(name != nullptr);
    // STARFISH_LOG_INFO("element %p animationStart: animationName [%s]",
    // element, name->toUTF8NonGCString().data());
    AnimationEventInit init;
    init.setAnimationName(name);
    if (delay < 0) {
        init.setElapsedTime(-(delay / 1000));
    } else {
        init.setElapsedTime(0);
    }
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    AnimationEvent* event = new AnimationEvent(
        element->executionContext(),
        element->starfish()->staticStrings()->m_animationstart.localName(),
        init);
    element->dispatchEventIdleTimeByUA(event);
}

void AnimationExecutor::fireAnimationEndEvent(Element* element, String* name,
                                              double elapsedTime)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(name != nullptr);
    // STARFISH_LOG_INFO("element %p animationEnd: animationName [%s]",
    // element, name->toUTF8NonGCString().data());
    AnimationEventInit init;
    init.setAnimationName(name);
    init.setElapsedTime(elapsedTime);
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    AnimationEvent* event = new AnimationEvent(
        element->executionContext(),
        element->starfish()->staticStrings()->m_animationend.localName(), init);
    element->dispatchEventIdleTimeByUA(event);
}

void AnimationExecutor::fireAnimationCancelEvent(Element* element, String* name,
                                                 double elapsedTime)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(name != nullptr);
    // STARFISH_LOG_INFO("element %p animationCancel: animationName [%s]",
    // element, name->toUTF8NonGCString().data());
    AnimationEventInit init;
    init.setAnimationName(name);
    init.setElapsedTime(elapsedTime);
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    AnimationEvent* event = new AnimationEvent(
        element->executionContext(),
        element->starfish()->staticStrings()->m_animationcancel.localName(),
        init);
    element->dispatchEventIdleTimeByUA(event);
}

void AnimationExecutor::fireSVGAnimateBeginEvent(Element* element)
{
    String* eventType =
        element->starfish()->staticStrings()->m_beginEvent.localName();
    Event* e = new Event(element->executionContext(), eventType,
                         EventInit(false, false));
    element->EventTarget::dispatchEventIdleTimeByUA(e);
}

void AnimationExecutor::fireSVGAnimateEndEvent(Element* element)
{
    String* eventType =
        element->starfish()->staticStrings()->m_endEvent.localName();
    Event* e = new Event(element->executionContext(), eventType,
                         EventInit(false, false));
    element->EventTarget::dispatchEventIdleTimeByUA(e);
}

} // namespace Starfish
