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
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/animation/AnimationTask.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/style/CalcData.h"
#include "core/animation/util/AnimationUtil.h"
#include "core/dom/HTMLHtmlElement.h"

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
    // CSS animation has mulitple AnimationKeyframes
    // i means animation index.
    for (size_t i = 0; i < styleAnimationData->animationKeyframesListSize();
         i++) {
        bool hasAnimatedProperty = false;
        const AnimationKeyframes& currentKeyFrames =
            styleAnimationData->animationKeyframes(i);

        if (!currentKeyFrames.isValid()) {
            continue;
        }

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
            for (auto* animationKeyframe :
                 currentKeyFrames.animationKeyframeList()) {
                offsets.push_back(animationKeyframe->keyframeSelector());
                // Legacy
                // TODO: It seems that one timing function is used for each
                // animation, but I don't think there is a need to save it as a
                // vector for each keyframe.
                timingFunctions.push_back(animationKeyframe->timingFunction());
            }

            hasAnimatedProperty |= applyProperty(
                i, currentKeyFrames.name(), currentKeyKind, layeredValues,
                layeredValues.size(), offsets, timingFunctions,
                currentKeyFrames.duration().toTimeValue(),
                currentKeyFrames.delay().toTimeValue(),
                currentKeyFrames.iterationCount(), currentKeyFrames.direction(),
                currentKeyFrames.playState(), currentKeyFrames.fillMode());
        }

        if (hasAnimatedProperty) {
            m_executor->fireAnimationStartEvent(
                m_element, currentKeyFrames.name(),
                currentKeyFrames.delay().toTimeValue());
            hasAppliedAnimation = true;
        }
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

        // TODO: I don't understand why this is necessary.
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

// TODO: There are so many parameters that it's so annoying.
bool AnimationApplier::applyProperty(
    size_t s, String* name, CSSStyleValuePair::KeyKind keyKind,
    const GCVector<GCVector<AnimatedValue*>>& values, size_t layerSize,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t duration,
    int64_t delay, float iterationCount, AnimationDirectionValue direction,
    AnimationPlayStateValue playState, AnimationFillModeValue fillMode)
{
    // TODO: This comes from AnimationTask. below verbose if-else statement
    // can be refactored

    bool gotAnimation = false;
    // color series
    if (AnimationUtil::checkCSSProperty(keyKind,
                                        CSSStyleValuePair::BackgroundColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::BackgroundColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BackgroundColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderBottomColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::BorderBottomColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderBottomColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderLeftColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::BorderLeftColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderLeftColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderRightColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::BorderRightColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderRightColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderTopColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::BorderTopColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderTopColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Color)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::Color, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Color);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::CaretColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::CaretColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::CaretColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::OutlineColor)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::OutlineColor, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::OutlineColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::TextDecorationColor,
                   CSSStyleValuePair::TextDecoration)) {
        auto task = new ActiveColorAnimationTask(
            m_element, CSSStyleValuePair::TextDecorationColor, values[0],
            offsets, timingFunctions, duration, delay, iterationCount,
            playState, fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::TextDecorationColor);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
        // <- color series
    } else if (keyKind == CSSStyleValuePair::Width) { // length series

        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Width, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Width);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::Height) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Height, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Height);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MinWidth) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MinWidth, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MinWidth);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MaxWidth) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MaxWidth, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MaxWidth);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MinHeight) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MinHeight, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MinHeight);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (keyKind == CSSStyleValuePair::MaxHeight) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MaxHeight, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MaxHeight);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::MarginTop)) {
        if (m_style->display() == DisplayValue::InlineDisplayValue) {
            STARFISH_UNIMPLEMENTED("InlineDisplayValue");
            return false;
        }
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginTop, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginTop);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::MarginRight)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginRight, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginRight);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::MarginBottom)) {
        if (m_style->display() == DisplayValue::InlineDisplayValue) {
            STARFISH_UNIMPLEMENTED("InlineDisplayValue");
            return false;
        }
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginBottom, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginBottom);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::MarginLeft)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginLeft, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::MarginLeft);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderTopWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderTopWidth, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderTopWidth);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderRightWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderRightWidth, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderRightWidth);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderBottomWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderBottomWidth, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderBottomWidth);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BorderLeftWidth)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderLeftWidth, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::BorderLeftWidth);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::PaddingTop)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingTop, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingTop);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::PaddingRight)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingRight, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingRight);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::PaddingBottom)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingBottom, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingBottom);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::PaddingLeft)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingLeft, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::PaddingLeft);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::RX)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::RX, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::RX);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::RY)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::RY, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::RY);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::CX)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::CX, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::CX);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::KeyKind::CY)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::KeyKind::CY, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::KeyKind::CY);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Left)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Left, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Left);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Right)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Right, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Right);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Top)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Top, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Top);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if ((AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::Bottom)) == true) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Bottom, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(m_element,
                                                     CSSStyleValuePair::Bottom);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(
                   keyKind, CSSStyleValuePair::BackgroundPositionX)) {
        if (m_style->hasBlockLikeDisplay() == false) {
            // TODO Inline Element
            STARFISH_UNIMPLEMENTED("Inline Element");
            return false;
        }
        for (size_t l = 0; l < layerSize; l++) {
            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::BackgroundPositionX, values[l],
                offsets, timingFunctions, duration, delay, iterationCount,
                playState, fillMode, l);
            m_executor->removeActiveAnimationTaskIfNeeds(
                m_element, CSSStyleValuePair::BackgroundPositionX, l);
            m_executor->registerAnimation(task, m_style, name, s,
                                          iterationCount, direction, playState,
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
        for (size_t l = 0; l < layerSize; l++) {
            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::BackgroundPositionY, values[l],
                offsets, timingFunctions, duration, delay, iterationCount,
                playState, fillMode, l);
            m_executor->removeActiveAnimationTaskIfNeeds(
                m_element, CSSStyleValuePair::BackgroundPositionY, l);
            m_executor->registerAnimation(task, m_style, name, s,
                                          iterationCount, direction, playState,
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
        for (size_t l = 0; l < layerSize; l++) {
            auto task = new ActiveLengthSizeAnimationTask(
                m_element, CSSStyleValuePair::BackgroundSize, values[l],
                offsets, timingFunctions, duration, delay, iterationCount,
                playState, fillMode, l);
            m_executor->removeActiveAnimationTaskIfNeeds(
                m_element, CSSStyleValuePair::BackgroundSize, l);
            m_executor->registerAnimation(task, m_style, name, s,
                                          iterationCount, direction, playState,
                                          m_isCSSAnimationTask);
        }
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::FontSize,
                                               CSSStyleValuePair::Font)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::FontSize, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::FontSize);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Opacity)) {
        auto task = new ActiveOpacityAnimationTask(
            m_element, CSSStyleValuePair::Opacity, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::Opacity);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Transform)) {
        auto task = new ActiveTransformAnimationTask(
            m_element, CSSStyleValuePair::Transform, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::Transform);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    } else if (AnimationUtil::checkCSSProperty(keyKind,
                                               CSSStyleValuePair::Visibility)) {
        auto task = new ActiveVisibilityAnimationTask(
            m_element, CSSStyleValuePair::Visibility, values[0], offsets,
            timingFunctions, duration, delay, iterationCount, playState,
            fillMode);
        m_executor->removeActiveAnimationTaskIfNeeds(
            m_element, CSSStyleValuePair::Visibility);
        m_executor->registerAnimation(task, m_style, name, s, iterationCount,
                                      direction, playState,
                                      m_isCSSAnimationTask);
        gotAnimation = true;
    }

    return gotAnimation;
}

} // namespace Starfish
