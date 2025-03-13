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

#include "AnimationApplier.h"

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

static bool isAnimatableBackgroundProperty(CSSStyleValuePair::KeyKind property)
{
    switch (property) {
    // Shorthand
    case CSSStyleValuePair::Background:
    case CSSStyleValuePair::BackgroundPosition:
    case CSSStyleValuePair::BackgroundPositionX:
    case CSSStyleValuePair::BackgroundPositionY:
    case CSSStyleValuePair::BackgroundSize:
        return true;
    default:
        break;
    }
    return false;
}

AnimationApplier::AnimationApplier(
    Element* element, AnimationType animatoinType, ComputedStyle* style,
    Optional<SVGAnimationElement*> originAnimationElement)
    : m_element(element)
    , m_animatoinType(animatoinType)
    , m_style(style)
    , m_originAnimationElement(originAnimationElement)
    , m_font(style->font())
    , m_currentFontSize(m_style->fontSize())
    , m_windowSize(element->window()->innerWidth(),
                   element->window()->innerHeight())
    , m_executor(element->document()->animationExecutor())
{
    m_rootFontSize = Length(
        Length::Fixed, m_element->document()->webView()->defaultFontSize());
    HTMLHtmlElement* root = m_element->document()->rootElement();
    if (root && root->style()) {
        m_rootFontSize = root->style()->fontSize();
    }
}

bool AnimationApplier::apply()
{
    bool hasAppliedAnimation = false;
    if (m_style->animation() == nullptr) {
        return false;
    }

    StyleAnimationData* styleAnimationData = m_style->animation();
    // CSS animation has mulitple AnimationKeyframes such as,
    // animation: x-animation 5s, r-animation 5s;
    //
    // i means animation index.
    for (size_t i = 0; i < styleAnimationData->animationKeyframesListSize();
         i++) {
        if (!styleAnimationData->isValidToApply(i)) {
            continue;
        }

        bool hasAnyAnimatedProperty = false;
        AnimationKeyframes& currentKeyFrames =
            styleAnimationData->animationKeyframes(i);

        // All AnimationKeyframe in animationKeyframeList have the same CSS
        // properties kind in the same order for generating animation tasks.
        // Therefore, based on the CSS properties of a from AnimationKeyframe,
        // create animation values for each property in each AnimationKeyframe.
        AnimationKeyframe* fromAnimationKeyframe =
            currentKeyFrames.animationKeyframeList()[0];

        // j means property index.
        for (size_t j = 0; j < fromAnimationKeyframe->propertySize(); j++) {
            CSSStyleValuePair::KeyKind currentKeyKind =
                fromAnimationKeyframe->keyKinds()[j];

            GCVector<GCVector<AnimatedValue*>> layeredValues;
            if (!createLayerdValues(&currentKeyFrames, currentKeyKind, j,
                                    layeredValues)) {
                // Failed to create AnimatedValue.
                continue;
            }

            GCAtomicVector<double> offsets;
            GCVector<TimingFunction*> timingFunctions;
            createOffsetAndTimingFunction(&currentKeyFrames, j, offsets,
                                          timingFunctions);
            STARFISH_ASSERT(layeredValues[0].size() == offsets.size());
            STARFISH_ASSERT(offsets.size() == timingFunctions.size());

            hasAnyAnimatedProperty |=
                applyProperty(i, currentKeyFrames.name(), currentKeyKind,
                              layeredValues, offsets, timingFunctions,
                              styleAnimationData->duration(i).toTimeValue(),
                              styleAnimationData->delay(i).toTimeValue(),
                              styleAnimationData->iterationCount(i),
                              styleAnimationData->direction(i),
                              styleAnimationData->playState(i),
                              styleAnimationData->fillMode(i));
        }

        if (hasAnyAnimatedProperty) {
            // Note that this originated from legacy code.
            double delay = currentKeyFrames.delay().toTimeValue();
            if (delay < 0) {
                delay = -(delay / 1000);
            } else {
                delay = 0;
            }

            m_executor->fireKeyFramesAnimationEvent(
                KeyFramesAnimationEventType::AnimationStart, m_element,
                currentKeyFrames.name(), delay);
            hasAppliedAnimation = true;
        }
    }
    return hasAppliedAnimation;
}

bool AnimationApplier::applySVGAnimateElement()
{
    STARFISH_ASSERT(m_originAnimationElement.hasValue());
    STARFISH_ASSERT(
        m_originAnimationElement.getValue()->isSVGAnimationElement());

    Optional<AnimationKeyframes*> maybekeyFrames =
        m_originAnimationElement.getValue()
            ->asSVGAnimationElement()
            ->animationKeyframes();
    if (!maybekeyFrames) {
        return false;
    }

    AnimationKeyframes* currentKeyFrames = maybekeyFrames.value();
    AnimationKeyframe* fromAnimationKeyframe =
        currentKeyFrames->animationKeyframeList()[0];
    bool hasAppliedAnimation = false;
    for (size_t i = 0; i < fromAnimationKeyframe->propertySize(); i++) {
        CSSStyleValuePair::KeyKind currentKeyKind =
            fromAnimationKeyframe->keyKinds()[i];

        GCVector<GCVector<AnimatedValue*>> layeredValues;
        if (!createLayerdValues(currentKeyFrames, currentKeyKind, i,
                                layeredValues)) {
            // Failed to create AnimatedValue.
            continue;
        }

        GCAtomicVector<double> offsets;
        GCVector<TimingFunction*> timingFunctions;
        createOffsetAndTimingFunction(currentKeyFrames, i, offsets,
                                      timingFunctions);
        STARFISH_ASSERT(layeredValues[0].size() == offsets.size());
        STARFISH_ASSERT(offsets.size() == timingFunctions.size());

        hasAppliedAnimation |= applyProperty(
            0, currentKeyFrames->name(), currentKeyKind, layeredValues, offsets,
            timingFunctions, currentKeyFrames->duration().toTimeValue(),
            currentKeyFrames->delay().toTimeValue(),
            currentKeyFrames->iterationCount(), currentKeyFrames->direction(),
            currentKeyFrames->playState(), currentKeyFrames->fillMode());
    }

    if (hasAppliedAnimation) {
        m_executor->fireSVGAnimationEvent(m_originAnimationElement.getValue(),
                                          SVGAnimationEventType::BeginEvent);
    }

    return hasAppliedAnimation;
}

bool AnimationApplier::createLayerdValues(
    const AnimationKeyframes* currentKeyFrames,
    CSSStyleValuePair::KeyKind currentKeyKind, size_t currentPropertyIndex,
    GCVector<GCVector<AnimatedValue*>>& layeredValues)
{
    size_t layerSize = 1;
    // TODO: there is another property that has layers
    if (isAnimatableBackgroundProperty(currentKeyKind)) {
        layerSize = m_style->backgroundLayerSize();
    }
    layeredValues.resize(layerSize);

    // layer means to the layer index and is for properties that have a
    // layer, such as the background. Otherwise it is 1.
    for (size_t layer = 0; layer < layerSize; layer++) {
        bool isAvailable = true;
        GCVector<AnimatedValue*> values;
        if (!createValues(currentKeyFrames, currentKeyKind,
                          currentPropertyIndex, layer, values)) {
            return false;
        }
        layeredValues[layer] = std::move(values);
    }

#ifdef STARFISH_ENABLE_TEST
    size_t valuesSize = layeredValues[0].size();
    for (size_t i = 1; i < layeredValues.size(); i++) {
        STARFISH_ASSERT(valuesSize == layeredValues[i].size());
    }
#endif

    return true;
}

bool AnimationApplier::createValues(const AnimationKeyframes* currentKeyFrames,
                                    CSSStyleValuePair::KeyKind currentKeyKind,
                                    size_t currentPropertyIndex, size_t layer,
                                    GCVector<AnimatedValue*>& values)
{
    // Property values ​​corresponding to each AnimationKeyframe
    // For example:
    // @keyframes rx-animation {
    //     0% {
    //         left: 0px;
    //     }
    //     50% {
    //         left: 200px;
    //     }
    //     100% {
    //         left: 400px;
    //     }
    // }
    // values: 0, 200, 400;
    for (auto* animationKeyframe : currentKeyFrames->animationKeyframeList()) {
        auto property = animationKeyframe->properties()[currentPropertyIndex];
        auto keyKind = animationKeyframe->keyKinds()[currentPropertyIndex];
        STARFISH_ASSERT(currentKeyKind == keyKind);

        if (isIntermediateDummyAnimationKeyframe(
                animationKeyframe, property.valueKind(), currentKeyFrames)) {
            continue;
        }

        bool neededOriginProperty = false;
        if (property.keyKind() == CSSStyleValuePair::KeyKind::Unknown &&
            keyKind != CSSStyleValuePair::KeyKind::Unknown) {
            neededOriginProperty = true;
        }

        Optional<AnimatedValue*> maybeAnimatedValue = AnimatedValue::create(
            m_style, m_element, property, keyKind, layer, neededOriginProperty);
        if (!maybeAnimatedValue) {
            return false;
        }

        AnimatedValue* animatedValue = maybeAnimatedValue.value();
        animatedValue->changeToFixedIfNeeded(m_currentFontSize, m_rootFontSize,
                                             m_font, m_windowSize.width(),
                                             m_windowSize.height(), nullptr);
        values.push_back(animatedValue);
    }

    return true;
}

void AnimationApplier::createOffsetAndTimingFunction(
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

bool AnimationApplier::isIntermediateDummyAnimationKeyframe(
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

void AnimationApplier::updateActiveAnimationTaskRegistration(
    size_t s, String* name, CSSStyleValuePair::KeyKind keyKind, size_t layer,
    float iterationCount, AnimationDirectionValue direction,
    AnimationPlayStateValue playState, ActiveAnimationTask* task)
{
    m_executor->removeActiveAnimationTaskIfNeeds(m_element, m_animatoinType,
                                                 keyKind, layer);
    m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                  playState, m_animatoinType);
}

bool AnimationApplier::applyProperty(
    size_t s, String* name, CSSStyleValuePair::KeyKind keyKind,
    const GCVector<GCVector<AnimatedValue*>>& layeredValues,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t duration,
    int64_t delay, float iterationCount, AnimationDirectionValue direction,
    AnimationPlayStateValue playState, AnimationFillModeValue fillMode)
{
    // TODO: There are so many parameters that it's so annoying.
    // TODO: This comes from AnimationTask. below verbose if-else statement
    // can be refactored.
    // It would be so grateful if you could do this.
    bool gotAnimation = false;

    // Convert shorthand to longhand.
    //
    // FIXME: This originated from legacy code. In my opinion, shorthand
    // properties should be decomposed into longhand properties to
    // arrive here.
    if (keyKind == CSSStyleValuePair::TextDecoration) {
        keyKind = CSSStyleValuePair::TextDecorationColor;
    } else if (keyKind == CSSStyleValuePair::Font) {
        keyKind = CSSStyleValuePair::FontSize;
    }

    ActiveAnimationTaskInit init;
    init.animationType = m_animatoinType;
    init.target = m_element;
    init.targetProperty = keyKind;
    init.playState = playState;
    init.fillMode = fillMode;
    init.durationInMs = duration;
    init.delayInMs = delay;
    init.iterationCount = iterationCount;
    init.offsets = offsets;
    init.timingFunctions = timingFunctions;

    if (m_originAnimationElement.hasValue()) {
        STARFISH_ASSERT(m_animatoinType == AnimationType::SVGAnimation);
        init.originAnimationElement = m_originAnimationElement;
    }

    for (size_t i = 0; i < layeredValues.size(); i++) {
        ActiveAnimationTask* task = nullptr;
        init.animatedValues = layeredValues[i];
        init.layerIndex = i;
        // Create ActiveAnimationTask based on the type of property.
        if (AnimationUtil::isPropertyForActiveColorAnimationTask(keyKind)) {
            task = new ActiveColorAnimationTask(init);

        } else if (AnimationUtil::isPropertyForActiveLengthAnimationTask(
                       keyKind)) {
            if (keyKind == CSSStyleValuePair::KeyKind::MarginTop ||
                keyKind == CSSStyleValuePair::KeyKind::MarginBottom) {
                if (m_style->display() == DisplayValue::InlineDisplayValue) {
                    // FIXME: This originated from legacy code.
                    STARFISH_UNIMPLEMENTED("InlineDisplayValue");
                    continue;
                }
            } else if (keyKind == CSSStyleValuePair::BackgroundPositionX ||
                       keyKind == CSSStyleValuePair::BackgroundPositionY) {
                if (m_style->hasBlockLikeDisplay() == false) {
                    // FIXME: This originated from legacy code.
                    STARFISH_UNIMPLEMENTED("Inline Element");
                    continue;
                }
            }
            task = new ActiveLengthAnimationTask(init, nullptr);
        } else if (AnimationUtil::isPropertyForActiveLengthSizeAnimationTask(
                       keyKind)) {
            if (!m_style->hasBlockLikeDisplay()) {
                // FIXME: This originated from legacy code.
                STARFISH_UNIMPLEMENTED("Inline Element");
                continue;
            }
            task = new ActiveLengthSizeAnimationTask(init, nullptr);
        } else if (keyKind == CSSStyleValuePair::Opacity) {
            task = new ActiveOpacityAnimationTask(init);

        } else if (keyKind == CSSStyleValuePair::Transform) {
            task = new ActiveTransformAnimationTask(init, nullptr);
        } else if (keyKind == CSSStyleValuePair::Visibility) {
            task = new ActiveVisibilityAnimationTask(init);
        }

        // Register ActiveAnimationTask.
        if (task) {
            updateActiveAnimationTaskRegistration(s, name, keyKind, i,
                                                  iterationCount, direction,
                                                  playState, task);
            gotAnimation = true;
        }
    }
    return gotAnimation;
}

} // namespace Starfish
