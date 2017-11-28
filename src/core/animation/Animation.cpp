/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/animation/Animation.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/TransitionEvent.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/style/ComputedStyle.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

AnimationTask::AnimationTask(Element* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             String* targetPropertyString, AnimatedValue from,
                             AnimatedValue to, float durationInms,
                             float delayInms,
                             AnimationTimingFunction* timingFunction)
{
    m_isExpired = false;
    m_isStarted = false;
    m_targetElement = target;
    m_durationMs = durationInms;
    m_delayMs = delayInms;
    m_lastModifiedTimeMs = m_startTimeMs = tickCount() + m_delayMs;
    m_fromValue = from;
    m_toValue = to;
    m_property = targetProperty;
    m_timingFunction = timingFunction;
    m_targetPropertyString = targetPropertyString;
}

// This function update last execution time.
// * This function is called before calling step() function.
void AnimationTask::update()
{
    m_lastModifiedTimeMs = tickCount();
}

void AnimationTask::fireStartEventIfNeeds()
{
    if (!m_isStarted) {
        TransitionEventInit init;
        init.setPropertyName(m_targetPropertyString);
        init.setBubbles(true);
        init.setCancelable(false);
        // TODO add more information to init
        TransitionEvent* event = new TransitionEvent(
            m_targetElement->document(), m_targetElement->starFish()
                                             ->staticStrings()
                                             ->m_transitionstart.localName(),
            init);
        m_targetElement->dispatchEventByUA(event);
        m_isStarted = true;
    }
}

void AnimationTask::fireEndEvent()
{
    detachedFromElement();

    TransitionEventInit init;
    init.setPropertyName(m_targetPropertyString);
    init.setBubbles(true);
    init.setCancelable(true);
    // TODO add more information to init
    TransitionEvent* event = new TransitionEvent(
        m_targetElement->document(), m_targetElement->starFish()
                                         ->staticStrings()
                                         ->m_transitionend.localName(),
        init);
    m_targetElement->dispatchEventByUA(event);
}

void AnimationTask::fireCancelEvent()
{
    detachedFromElement();

    TransitionEventInit init;
    init.setPropertyName(m_targetPropertyString);
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    TransitionEvent* event = new TransitionEvent(
        m_targetElement->document(), m_targetElement->starFish()
                                         ->staticStrings()
                                         ->m_transitioncancel.localName(),
        init);
    m_targetElement->dispatchEventByUA(event);
}

// This function returns false if
// * Current AnimationTask is expired, OR
// * Minimum period(THRESHOLD_TICK,10ms) has not passed yet
// * This function is called when we get tick from ecore_animator.
bool AnimationTask::canExecute()
{
    if (isExpired()) {
        return false;
    }
    size_t currentTime = tickCount();
    if ((currentTime >= m_startTimeMs)) {
        if (m_lastModifiedTimeMs == 0 ||
            ((currentTime - m_lastModifiedTimeMs) > THRESHOLD_TICK)) {
            update();
            return true;
        }
    }
    return false;
}

// This function calculte progress value to get intermediate value of animation.
// * returnVal range : 0-1
float AnimationTask::progress()
{
    if (m_durationMs == 0 || isExpired()) {
        return 1;
    }
    auto timeDiff = std::max((size_t)1, m_lastModifiedTimeMs - m_startTimeMs);
    float result = timeDiff / ((float)m_durationMs);
    if (result >= 1) {
        result = 1;
        m_isExpired = true;
    }
    result = m_timingFunction->getValue(result);
    return result;
}

// This function change computed style of target node.
// * After this function, NeedsPainting flag will be set.
void ColorAnimationTask::execute()
{
    Unit::Color from = m_fromValue.getColor();
    Unit::Color to = m_toValue.getColor();

    Element* current = targetElement();
    ComputedStyle* style = current->style();
    float tmp_progress = progress();

    // It is arbitrary logic which I made for testing
    unsigned char r = from.r() * (1 - tmp_progress) + to.r() * tmp_progress;
    unsigned char g = from.g() * (1 - tmp_progress) + to.g() * tmp_progress;
    unsigned char b = from.b() * (1 - tmp_progress) + to.b() * tmp_progress;
    unsigned char a = from.a();

    // TODO : More types should be supported
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        style->setBackgroundColor(Unit::Color(r, g, b, a));
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    current->setNeedsPainting();
}

// This function change computed style of target node.
// * After this function, NeedsLayout flag will be set.
void LengthAnimationTask::execute()
{
    Length from = m_fromValue.getLength();
    Length to = m_toValue.getLength();

    Element* current = targetElement();
    ComputedStyle* style = current->style();
    float tmpProgress = progress();
    float newLength;
    if (from.fixed() < to.fixed()) {
        newLength = from.fixed() + (to.fixed() - from.fixed()) * tmpProgress;
    } else {
        newLength = from.fixed() - (from.fixed() - to.fixed()) * tmpProgress;
    }

    // TODO : More types should be supported
    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        style->setWidth(Length(Length::Fixed, newLength));
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        style->setHeight(Length(Length::Fixed, newLength));
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    current->setNeedsLayout();
}
void TransformAnimationTask::computeToValue()
{
    FrameBox* box = targetElement()->frame()->asFrameBox();
    SkMatrix matrix = box->style()->transformsToMatrix(
        box->width(), box->height(), box, true);
    m_toValue = AnimatedValue(matrix);

    Element* current = targetElement();
    ComputedStyle* style = current->style();

    if (!style->hasTransforms()) {
        style->setRareComputedStyleDataIfNeeded();
        // NOTE
        // having transform is reason of creating StackingContext
        // for rebuilding stacking context, we should give layout damage
        current->setNeedsLayout();
    }
    style->rareComputedStyleData()->ensureTransforms()->append(
        StyleTransformData(StyleTransformData::InternalMatrix));
}

void TransformAnimationTask::setup()
{
    computeToValue();
    // calling execute function explicity for setting initial value of
    // ComputedStyle
    execute();
}

void TransformAnimationTask::attachedToElement()
{
    AnimationTask::attachedToElement();
    if (targetElement()->frame()) {
        targetElement()->frame()->markRunningTransformAnimation();
    }
}

void TransformAnimationTask::detachedFromElement()
{
    AnimationTask::detachedFromElement();
    if (targetElement()->frame()) {
        targetElement()->frame()->clearRunningTransformAnimation();
    }
}

void TransformAnimationTask::execute()
{
    SkMatrix from = m_fromValue.getMatrix();
    SkMatrix to = m_toValue.getMatrix();

    Element* current = targetElement();
    ComputedStyle* style = current->style();
    float tmpProgress = progress();

    auto transforms = style->rareComputedStyleData()->transforms();
    if (isExpired()) {
        // cleanup
        if (transforms->at(transforms->size() - 1).type() ==
            StyleTransformData::InternalMatrix) {
            transforms->removeAt(transforms->size() - 1);
            if (!style->hasTransforms()) {
                // NOTE
                // having transform is reason of creating StackingContext
                // for rebuilding stacking context, we should give layout damage
                current->setNeedsLayout();
            }
        }
    } else {
        if (transforms->at(transforms->size() - 1).type() !=
            StyleTransformData::InternalMatrix) {
            computeToValue();
        }
        StyleTransformData& data = transforms->at(transforms->size() - 1);
        SkMatrix now;
        for (size_t i = 0; i < 9; i++) {
            double d =
                from.get(i) * (1 - tmpProgress) + to.get(i) * tmpProgress;
            now.set(i, d);
        }

        data.setInternalMatrix(now);
    }
    current->webView()->setNeedsComputeStackingContextProperties();
}

// [NOTICE]
// registerAnimation will be replaced 'createAnimation'
// Creation of AnimationTask will happen in Animation Executor.
void AnimationExecutor::registerAnimation(AnimationTask* newTask)
{
    startIfNeeds();
    cancelPreviousAnimation(newTask->targetElement(), newTask->propertyType());
    newTask->attachedToElement();
    m_animationList.push_back(newTask);
}

// This function cancel previous animation
// which is related taget node and its property.
// * This function is called when we need new animation.
void AnimationExecutor::cancelPreviousAnimation(
    Element* target, CSSStyleValuePair::KeyKind cssType)
{
    // TODO : Need optimization
    // fire end event
    m_animationList.erase(
        std::remove_if(m_animationList.begin(), m_animationList.end(),
                       [&target, cssType](AnimationTask* current) {
                           if (current->targetElement() == target &&
                               current->propertyType() == cssType) {
                               current->fireCancelEvent();
                               return true;
                           }
                           return false;
                       }),
        m_animationList.end());
}

// This function clear All animation which is related target node.
void AnimationExecutor::cancelAnimation(Element* target)
{
    m_animationList.erase(
        std::remove_if(m_animationList.begin(), m_animationList.end(),
                       [&target](AnimationTask* current) {
                           if (current->targetElement() == target) {
                               current->fireCancelEvent();
                               return true;
                           }
                           return false;
                       }),
        m_animationList.end());
}

// [NOTICE]
// * Currently, Its behavior hardly coupled with ecore_animator
// * I believe it's best choice at this moment.
// * We do not have a sophisticated solution that surpasses this
void AnimationExecutor::startIfNeeds()
{
    if (m_isAlive && m_platformAnimator) {
        return;
    }
    m_isAlive = true;
    m_platformAnimator = window()->starFish()->timer()->addAnimator(
        window(),
        [](void* data) -> bool {
            AnimationExecutor* executor = (AnimationExecutor*)data;
            if (executor->isAlive()) {
                executor->step();
                return true;
            }
            return false;
        },
        this);
}

void AnimationExecutor::stop()
{
    if (!m_isAlive) {
        return;
    }
    m_isAlive = false;
    if (m_platformAnimator) {
        window()->starFish()->timer()->removeGenericAnimator(
            m_platformAnimator);
        m_platformAnimator = 0;
    }
}

void AnimationExecutor::stopIfNeeds()
{
    if (m_animationList.size() == 0) {
        stop();
    }
}

// [NOTICE]
// * Basically, Do execute every task at one tick.
// * In reality, there will not be many works to be done(means number of
// parallel animations)
// * And Calculating and Changing computed style is pretty light work.
// * If we spent a lot of time during other stuff, MessageLoop will adjust
// next execution.
void AnimationExecutor::step()
{
    STARFISH_ASSERT(m_isAlive);
    for (size_t i = 0; i < m_animationList.size(); i++) {
        AnimationTask* task = m_animationList[i];
        if (task->isExpired()) {
            task->fireEndEvent();
            m_animationList.erase(i);
            i--;
        } else {
            if (task->canExecute()) {
                task->fireStartEventIfNeeds();
                task->execute();
            }
        }
    }
    stopIfNeeds();
}
}
