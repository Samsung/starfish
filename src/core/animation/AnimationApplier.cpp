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
#include "core/dom/svg/SVGAnimateElement.h"

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

AnimationApplier::AnimationApplier(Element* element, ComputedStyle* style,
                                   bool isCSSAnimationTask)
    : m_element(element)
    , m_style(style)
    , m_isCSSAnimationTask(isCSSAnimationTask)
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
            m_executor->fireAnimationStartEvent(
                m_element, currentKeyFrames.name(),
                currentKeyFrames.delay().toTimeValue());
            hasAppliedAnimation = true;
        }
    }
    return hasAppliedAnimation;
}

bool AnimationApplier::applySVGAnimateElement(
    SVGAnimateElement* animationElement)
{
    Optional<AnimationKeyframes*> maybekeyFrames =
        animationElement->animationKeyframes();
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

    // ActiveColorAnimationTask
    if (AnimationUtil::isPropertyForActiveColorAnimationTask(keyKind)) {
        if (keyKind == CSSStyleValuePair::TextDecoration) {
            // FIXME: This originated from legacy code. In my opinion, shorthand
            // properties should be decomposed into longhand properties to
            // arrive here.
            keyKind = CSSStyleValuePair::TextDecorationColor;
        }
        auto task = new ActiveColorAnimationTask(
            m_element, keyKind, layeredValues[0], offsets, timingFunctions,
            duration, delay, iterationCount, playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element, keyKind);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::Width) { // length series

        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Width, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Width);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::Height) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Height, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Height);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MinWidth) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MinWidth, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MinWidth);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MaxWidth) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MaxWidth, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MaxWidth);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MinHeight) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MinHeight, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MinHeight);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MaxHeight) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MaxHeight, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MaxHeight);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::MarginTop)) {
        if (m_style->display() == DisplayValue::InlineDisplayValue) {
            STARFISH_UNIMPLEMENTED("InlineDisplayValue");
            return false;
        }
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginTop, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginTop);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::MarginRight)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginRight, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginRight);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::MarginBottom)) {
        if (m_style->display() == DisplayValue::InlineDisplayValue) {
            STARFISH_UNIMPLEMENTED("InlineDisplayValue");
            return false;
        }
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginBottom, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginBottom);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::MarginLeft)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginLeft, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginLeft);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderTopWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderTopWidth, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderTopWidth);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderRightWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderRightWidth, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderRightWidth);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderBottomWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderBottomWidth, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderBottomWidth);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderLeftWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderLeftWidth, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderLeftWidth);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::PaddingTop)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingTop, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingTop);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::PaddingRight)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingRight, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingRight);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::PaddingBottom)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingBottom, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingBottom);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::PaddingLeft)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingLeft, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingLeft);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::RX)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::RX, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::RX);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::RY)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::RY, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::RY);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::CX)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::CX, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::CX);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::CY)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::CY, layeredValues[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::CY);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Left)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Left, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Left);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Right)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Right, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Right);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Top)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Top, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Top);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Bottom)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Bottom, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Bottom);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BackgroundPositionX)) {
        if (m_style->hasBlockLikeDisplay() == false) {
            // TODO Inline Element
            STARFISH_UNIMPLEMENTED("Inline Element");
            return false;
        }
        for (size_t l = 0; l < layeredValues.size(); l++) {
            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::BackgroundPositionX,
                layeredValues[l], offsets, timingFunctions, duration, delay,
                iterationCount, playState, fillMode, l);
            m_executor->removeActiveAnimationTaskIfNeeds(
                m_element, CSSStyleValuePair::BackgroundPositionX, l);
            m_executor->registerAnimation(task, name, s, iterationCount,
                                          direction, playState,
                                          m_isCSSAnimationTask);
        }

        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BackgroundPositionY)) {
        if (m_style->hasBlockLikeDisplay() == false) {
            // TODO Inline Element
            STARFISH_UNIMPLEMENTED("Inline Element");
            return false;
        }
        for (size_t l = 0; l < layeredValues.size(); l++) {
            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::BackgroundPositionY,
                layeredValues[l], offsets, timingFunctions, duration, delay,
                iterationCount, playState, fillMode, l);
            m_executor->removeActiveAnimationTaskIfNeeds(
                m_element, CSSStyleValuePair::BackgroundPositionY, l);
            m_executor->registerAnimation(task, name, s, iterationCount,
                                          direction, playState,
                                          m_isCSSAnimationTask);
        }
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BackgroundSize)) {
        if (m_style->hasBlockLikeDisplay() == false) {
            // TODO Inline Element
            STARFISH_UNIMPLEMENTED("Inline Element");
            return false;
        }
        for (size_t l = 0; l < layeredValues.size(); l++) {
            auto task = new ActiveLengthSizeAnimationTask(
                m_element, CSSStyleValuePair::BackgroundSize, layeredValues[l],
                offsets, timingFunctions, duration, delay, iterationCount,
                playState, fillMode, l);
            m_executor->removeActiveAnimationTaskIfNeeds(
                m_element, CSSStyleValuePair::BackgroundSize, l);
            m_executor->registerAnimation(task, name, s, iterationCount,
                                          direction, playState,
                                          m_isCSSAnimationTask);
        }
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::FontSize,
                                               CSSStyleValuePair::Font)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::FontSize, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::FontSize);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Opacity)) {
        auto task = new ActiveOpacityAnimationTask(
            m_element, CSSStyleValuePair::Opacity, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::Opacity);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Transform)) {
        auto task = new ActiveTransformAnimationTask(
            m_element, CSSStyleValuePair::Transform, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::Transform);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Visibility)) {
        auto task = new ActiveVisibilityAnimationTask(
            m_element, CSSStyleValuePair::Visibility, layeredValues[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::Visibility);
        m_executor->registerAnimation(task, name, s, iterationCount, direction,
                                      playState, m_isCSSAnimationTask);
        gotAnimation = true;
    }

    return gotAnimation;
}

} // namespace Starfish
