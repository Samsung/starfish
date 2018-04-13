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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/animation/Animation.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/TransitionEvent.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/StackingContext.h"
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
    m_targetElement = target;
    m_durationMs = durationInms;
    m_delayMs = delayInms;
    m_startTimeMs = 0;
    m_fromValue = from;
    m_toValue = to;
    m_property = targetProperty;
    m_timingFunction = timingFunction;
    m_targetPropertyString = targetPropertyString;
}

void AnimationTask::attachedToElement()
{
    m_targetElement->style()->markUsedInAnimator();
}

void AnimationTask::fireStartEvent()
{
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

// This function calculte progress value to get intermediate value of animation.
// * returnVal range : 0-1
float AnimationTask::computeProgress(uint64_t tickCount)
{
    uint64_t timeDiff = tickCount - m_startTimeMs;
    float result = timeDiff / ((float)m_durationMs);
    if (result >= 1) {
        result = 1;
    }
    result = m_timingFunction->getValue(result);
    return result;
}

// This function change computed style of target node.
// * After this function, NeedsPainting flag will be set.
void ColorAnimationTask::execute(float progress)
{
    Unit::Color from = m_fromValue.getColor();
    Unit::Color to = m_toValue.getColor();

    Element* current = targetElement();
    ComputedStyle* style = current->style();

    // It is arbitrary logic which I made for testing
    unsigned char r = from.r() * (1 - progress) + to.r() * progress;
    unsigned char g = from.g() * (1 - progress) + to.g() * progress;
    unsigned char b = from.b() * (1 - progress) + to.b() * progress;
    unsigned char a = from.a();

    // TODO : More types should be supported
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        style->setBackgroundColor(Unit::Color(r, g, b, a));
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    current->setNeedsPainting();
}

// This function change computed style of target node.
// * After this function, NeedsLayout flag will be set.
void LengthAnimationTask::execute(float progress)
{
    Length from = m_fromValue.getLength();
    Length to = m_toValue.getLength();

    Element* current = targetElement();
    ComputedStyle* style = current->style();
    float newLength;
    if (from.fixed() < to.fixed()) {
        newLength = from.fixed() + (to.fixed() - from.fixed()) * progress;
    } else {
        newLength = from.fixed() - (from.fixed() - to.fixed()) * progress;
    }

    // TODO : More types should be supported
    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        style->setWidth(Length(Length::Fixed, newLength));
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        style->setHeight(Length(Length::Fixed, newLength));
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    current->setNeedsLayout();
}

TransformAnimationTask::TransformAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    String* targetPropertyString, AnimatedValue fromValue, float duration,
    float delay, AnimationTimingFunction* timingFunction)
    : AnimationTask(
          target, CSSStyleValuePair::KeyKind::Transform,
          transitionPropertyValueToString(TransitionPropertyTransformValue),
          fromValue, AnimatedValue(), duration, delay, timingFunction)
{
    target->style()->rareComputedStyleData()->ensureTransforms();
}

void TransformAnimationTask::computeToValue()
{
    FrameBox* box = targetElement()->frame()->asFrameBox();
    SkMatrix matrix = box->style()->transformsToMatrix(
        box->width(), box->height(), box, true);
    m_toValue = AnimatedValue(matrix);

    Element* current = targetElement();
    ComputedStyle* style = current->style();

    bool needsToRecomputeStackingContext = false;
    if (!style->hasTransforms() || style->transforms(box)->size() == 0) {
        // NOTE
        // having transform is reason of creating StackingContext
        needsToRecomputeStackingContext = true;
    }

    style->rareComputedStyleData()->ensureTransforms()->append(
        StyleTransformData(StyleTransformData::InternalMatrix));

    if (needsToRecomputeStackingContext) {
        box->computeStyleFlags();
        current->webView()->clearStackingContext(true);
    }

    execute(0);
}

void TransformAnimationTask::attachedToElement()
{
    AnimationTask::attachedToElement();
    targetElement()->markRunningTransformAnimation();
}

void TransformAnimationTask::detachedFromElement()
{
    AnimationTask::detachedFromElement();

    Element* current = targetElement();
    ComputedStyle* style = current->style();
    FrameBox* box = current->frame()->asFrameBox();

    if (style && style->hasTransforms()) {
        auto transforms = style->rareComputedStyleData()->transforms();
        if (transforms->at(transforms->size() - 1).type() ==
            StyleTransformData::InternalMatrix) {
            // cleanup
            transforms->removeAt(transforms->size() - 1);
            if (transforms->size() == 0) {
                current->webView()->clearStackingContext(true);
                style->clearTransform();
                box->computeStyleFlags();
            } else {
                current->webView()->setNeedsComputeStackingContextProperties();
                STARFISH_RELEASE_ASSERT(
                    transforms->at(transforms->size() - 1).type() !=
                    StyleTransformData::InternalMatrix);
            }
        }
    }

    current->clearRunningTransformAnimation();
}

void TransformAnimationTask::execute(float progress)
{
    SkMatrix from = m_fromValue.getMatrix();
    SkMatrix to = m_toValue.getMatrix();

    Element* current = targetElement();
    ComputedStyle* style = current->style();
    if (!style->rareComputedStyleData() ||
        style->rareComputedStyleData()->transforms() == nullptr ||
        style->rareComputedStyleData()->transforms()->size() == 0 ||
        style->rareComputedStyleData()
                ->transforms()
                ->at(style->rareComputedStyleData()->transforms()->size() - 1)
                .type() != StyleTransformData::InternalMatrix) {
        computeToValue();
    }
    auto transforms = style->rareComputedStyleData()->transforms();
    STARFISH_RELEASE_ASSERT(transforms->at(transforms->size() - 1).type() ==
                            StyleTransformData::InternalMatrix);

    StyleTransformData& data = transforms->at(transforms->size() - 1);
    SkMatrix now;
    for (size_t i = 0; i < 9; i++) {
        double d = from.get(i) * (1 - progress) + to.get(i) * progress;
        now.set(i, d);
    }

    data.setInternalMatrix(now);
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
// * I believe it's best choice at this moment.
// * We do not have a sophisticated solution that surpasses this
void AnimationExecutor::startIfNeeds()
{
    if (m_isAlive && m_platformAnimator != SIZE_MAX) {
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
    if (m_platformAnimator != SIZE_MAX) {
        window()->starFish()->timer()->removeGenericAnimator(
            m_platformAnimator);
        m_platformAnimator = SIZE_MAX;
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
    uint64_t currentTickCount = tickCount();

    for (size_t i = 0; i < m_animationList.size(); i++) {
        AnimationTask* task = m_animationList[i];

        if (task->m_startTimeMs == 0) {
            task->m_startTimeMs = currentTickCount;
            task->fireStartEvent();
        }

        float progress = task->computeProgress(currentTickCount);
        if (progress >= 1 || task->targetElement()->frame() == nullptr ||
            !task->targetElement()
                 ->isInDocumentScopeAndDocumentParticipateInRendering()) {
            task->fireEndEvent();
            m_animationList.erase(i);
            i--;
        } else {
            task->execute(progress);
        }
    }
    stopIfNeeds();
}

void AnimationExecutor::runPendingAnimation()
{
    for (size_t i = 0; i < m_pendingAnimationInfoList.size(); i++) {
        PendingAnimiationInfo* info = m_pendingAnimationInfoList[i];

        bool elementHasAnimation = false;
        for (size_t j = 0; j < m_animationList.size(); j++) {
            if (m_animationList[j]->targetElement() == info->element) {
                elementHasAnimation = true;
                break;
            }
        }

        if (elementHasAnimation) {
            continue;
        }

        ComputedStyle* currentElementStyle = info->element->style();
        if (!info->element
                 ->isInDocumentScopeAndDocumentParticipateInRendering()) {
            currentElementStyle = nullptr;
        }

        bool damagedKeys[CSSStyleValuePair::KeyKindSize] = {
            false,
        };

        if (currentElementStyle &&
            !info->newStyle->transitionDuration().isZero()) {
            compareStyle(info->oldStyle, currentElementStyle, damagedKeys);
            if (!info->oldFrame) {
                info->oldFrame = info->element->frame();
            }
            if (info->oldFrame) {
                applyTransition(info->element, info->oldStyle, info->oldFrame,
                                currentElementStyle, damagedKeys);
            }
        }
    }
    m_pendingAnimationInfoList.clear();
}

void AnimationExecutor::addPendingAnimation(Element* element,
                                            ComputedStyle* oldStyle,
                                            ComputedStyle* newStyle,
                                            Frame* oldFrame)
{
    oldStyle->markUsedInAnimator();
    newStyle->markUsedInAnimator();

    for (size_t i = 0; i < m_pendingAnimationInfoList.size(); i++) {
        if (m_pendingAnimationInfoList[i]->element == element) {
            m_pendingAnimationInfoList[i]->oldStyle = oldStyle;
            m_pendingAnimationInfoList[i]->newStyle = newStyle;
            m_pendingAnimationInfoList[i]->oldFrame = oldFrame;
            return;
        }
    }

    if (m_pendingAnimationInfoList.size() == 0) {
        window()->browsingContext()->notifyHasPendingAnimation();
    }

    PendingAnimiationInfo* info = new PendingAnimiationInfo();
    info->element = element;
    info->oldStyle = oldStyle;
    info->newStyle = newStyle;
    info->oldFrame = oldFrame;
    m_pendingAnimationInfoList.push_back(info);
}
}
