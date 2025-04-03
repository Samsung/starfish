/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "SVGAnimationApplier.h"

#include "core/animation/AnimationTask.h"
#include "core/animation/AnimationExecutor.h"
#include "core/animation/util/AnimationUtil.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/svg/SVGAnimationElement.h"

namespace Starfish {

SVGAnimationApplier::SVGAnimationApplier(
    Element* element, SVGAnimationElement* originAnimationElement)
    : m_element(element)
    , m_originAnimationElement(originAnimationElement)
    , m_executor(element->document()->animationExecutor())
{
}

bool SVGAnimationApplier::apply()
{
    STARFISH_ASSERT(m_originAnimationElement->isSVGAnimationElement());

    Optional<AnimationKeyframes*> maybekeyFrames =
        m_originAnimationElement->asSVGAnimationElement()->animationKeyframes();
    if (!maybekeyFrames) {
        return false;
    }

    AnimationKeyframes* currentKeyFrames = maybekeyFrames.value();
    AnimationKeyframe* fromAnimationKeyframe =
        currentKeyFrames->animationKeyframeList()[0];
    bool hasAppliedAnimation = false;
    for (size_t propertyIndex = 0;
         propertyIndex < fromAnimationKeyframe->propertySize();
         propertyIndex++) {
        CSSStyleValuePair::KeyKind currentKeyKind =
            fromAnimationKeyframe->keyKinds()[propertyIndex];

        GCVector<AnimatedValue*> values;
        if (!createValues(currentKeyFrames, currentKeyKind, propertyIndex,
                          values)) {
            return false;
        }

        GCAtomicVector<double> offsets;
        GCVector<TimingFunction*> timingFunctions;
        createOffsetAndTimingFunction(currentKeyFrames, propertyIndex, offsets,
                                      timingFunctions);
        STARFISH_ASSERT(offsets.size() == timingFunctions.size());

        hasAppliedAnimation |= applyProperty(
            currentKeyFrames->name(), currentKeyKind, values, offsets,
            timingFunctions, currentKeyFrames->duration().toTimeValue(),
            currentKeyFrames->delay().toTimeValue(),
            currentKeyFrames->iterationCount(), currentKeyFrames->direction(),
            currentKeyFrames->playState(), currentKeyFrames->fillMode());
    }

    if (hasAppliedAnimation) {
        m_executor->fireSVGAnimationEvent(m_originAnimationElement,
                                          SVGAnimationEventType::BeginEvent);
    }

    return hasAppliedAnimation;
}

bool SVGAnimationApplier::createValues(
    const AnimationKeyframes* currentKeyFrames,
    CSSStyleValuePair::KeyKind currentKeyKind, size_t currentPropertyIndex,
    GCVector<AnimatedValue*>& values)
{
    for (auto* animationKeyframe : currentKeyFrames->animationKeyframeList()) {
        auto property = animationKeyframe->properties()[currentPropertyIndex];
        auto keyKind = animationKeyframe->keyKinds()[currentPropertyIndex];
        STARFISH_ASSERT(currentKeyKind == keyKind);

        bool neededOriginProperty = false;
        if (property.keyKind() == CSSStyleValuePair::KeyKind::Unknown &&
            keyKind != CSSStyleValuePair::KeyKind::Unknown) {
            neededOriginProperty = true;
        }

        Optional<AnimatedValue*> maybeAnimatedValue =
            AnimatedValue::createForSVGAnimation(m_element, property, keyKind);
        if (!maybeAnimatedValue) {
            return false;
        }

        AnimatedValue* animatedValue = maybeAnimatedValue.value();
        values.push_back(animatedValue);
    }

    return true;
}

void SVGAnimationApplier::createOffsetAndTimingFunction(
    AnimationKeyframes* currentKeyFrames, size_t currentPropertyIndex,
    GCAtomicVector<double>& offsets, GCVector<TimingFunction*>& timingFunctions)
{
    for (auto* animationKeyframe : currentKeyFrames->animationKeyframeList()) {
        if (isIntermediateDummyAnimationKeyframe(
                animationKeyframe,
                animationKeyframe->properties()[currentPropertyIndex]
                    .valueKind(),
                currentKeyFrames)) {
            continue;
        }
        offsets.push_back(animationKeyframe->keyframeSelector());
        // Legacy
        // TODO: It seems that one timing function is used for each
        // animation, but I don't think there is a need to save it as a
        // vector for each keyframe.
        timingFunctions.push_back(animationKeyframe->timingFunction());
    }
}

bool SVGAnimationApplier::isIntermediateDummyAnimationKeyframe(
    AnimationKeyframe* current, CSSStyleValuePair::ValueKind valueKind,
    const AnimationKeyframes* owner)
{
    AnimationKeyframe* from = owner->animationKeyframeList().front();
    AnimationKeyframe* to = owner->animationKeyframeList().back();
    if ((current != from && current != to) &&
        valueKind == CSSStyleValuePair::ValueKind::None) {
        return true;
    }
    return false;
}

void SVGAnimationApplier::updateActiveAnimationTaskRegistration(
    size_t s, String* name, CSSStyleValuePair::KeyKind keyKind, size_t layer,
    float iterationCount, AnimationDirectionValue direction,
    AnimationPlayStateValue playState, ActiveAnimationTask* task)
{
    m_executor->removeActiveAnimationTaskIfNeeds(
        m_element, AnimationType::SVGAnimation, keyKind, layer);
    m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                  playState, AnimationType::SVGAnimation);
}

bool SVGAnimationApplier::applyProperty(
    String* name, CSSStyleValuePair::KeyKind keyKind,
    const GCVector<AnimatedValue*>& values,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t duration,
    int64_t delay, float iterationCount, AnimationDirectionValue direction,
    AnimationPlayStateValue playState, AnimationFillModeValue fillMode)
{
    bool gotAnimation = false;

    ActiveAnimationTaskInit init;
    init.animationType = AnimationType::SVGAnimation;
    init.target = m_element;
    init.targetProperty = keyKind;
    init.playState = playState;
    init.fillMode = fillMode;
    init.durationInMs = duration;
    init.delayInMs = delay;
    init.iterationCount = iterationCount;
    init.offsets = offsets;
    init.timingFunctions = timingFunctions;
    init.originAnimationElement = m_originAnimationElement;

    ActiveAnimationTask* task = nullptr;
    init.animatedValues = values;

    // TODO: Introduce new ActiveTasks for SVGAnimatedXX values if needs.
    // Perhaps the newly introduced task should change the animVal corresponding
    // to SVGAnimatedXX for each property instead of changing the computed style
    // value.

    // Create ActiveAnimationTask based on the type of property.
    if (AnimationUtil::isPropertyForActiveLengthAnimationTaskForSVG(keyKind)) {
        task = new ActiveLengthAnimationTask(init, nullptr);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Transform) {
        task = new ActiveTransformAnimationTask(init, nullptr);
    } else {
        STARFISH_UNIMPLEMENTED(
            "Unhandled property kind[%ud] for SVG animation.",
            static_cast<unsigned int>(keyKind));
    }

    // Register ActiveAnimationTask.
    if (task) {
        updateActiveAnimationTaskRegistration(
            0, name, keyKind, 0, iterationCount, direction, playState, task);
        gotAnimation = true;
    }

    return gotAnimation;
}

} // namespace Starfish
