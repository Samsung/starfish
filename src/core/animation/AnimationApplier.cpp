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

static void resolveLengthAnimatedValueIfNeeded(AnimatedValue* value, Font* font,
                                               const Length& curFontSize,
                                               const Length& rootFontSize,
                                               HTMLHtmlElement* root,
                                               const LayoutSize& windowSize)
{
    if (value->isLength()) {
        if (!value->getLength().isFixed()) {
            auto v = value->getLength();
            v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                    windowSize.width(), windowSize.height(),
                                    nullptr);
            value->setLength(v);
        }
    }
}

AnimationApplier::AnimationApplier(Element* element, ComputedStyle* style,
                                   bool isCSSAnimationTask)
    : m_element(element)
    , m_style(style)
    , m_isCSSAnimationTask(isCSSAnimationTask)
    , m_executor(element->document()->animationExecutor())
{
}

bool AnimationApplier::apply()
{
    bool ret = false;

    if (m_style->animation() == nullptr) {
        return false;
    }

    Font* font = m_style->font();
    Length curFontSize = m_style->fontSize();
    Length rootFontSize = Length(
        Length::Fixed, m_element->document()->webView()->defaultFontSize());
    HTMLHtmlElement* root = m_element->document()->rootElement();
    if (root && root->style()) {
        rootFontSize = root->style()->fontSize();
    }

    Window* w = m_element->window();
    LayoutSize windowSize(w->innerWidth(), w->innerHeight());

    StyleAnimationData* styleAnimationData = m_style->animation();
    for (size_t s = 0; s < styleAnimationData->keyframesListSize(); s++) {
        String* name = styleAnimationData->animationName(s);
        if (name->equals(String::emptyString) == true ||
            name->equalsIgnoreCase("none") == true) {
            continue;
        }

        double duration = styleAnimationData->duration(s).toTimeValue();
        if (duration == 0.0) {
            continue;
        }

        AnimationKeyframes& keyframes = styleAnimationData->keyframes(s);
        if (keyframes.keyframeList().size() == 0) {
            continue;
        }

        AnimationKeyframe* fromKeyframe = keyframes.keyframe(0);
        if (fromKeyframe == nullptr) {
            continue;
        }

        double delay = styleAnimationData->delay(s).toTimeValue();
        float iterationCount = styleAnimationData->iterationCount(s);
        AnimationDirectionValue direction = styleAnimationData->direction(s);
        AnimationPlayStateValue playState = styleAnimationData->playState(s);
        AnimationFillModeValue fillMode = styleAnimationData->fillMode(s);

        size_t keyframeSize = keyframes.keyframeListSize();
        bool neededOriginProperty = false;
        for (size_t i = 0; i < fromKeyframe->propertySize(); i++) {
            auto property = fromKeyframe->properties()[i];
            auto keyKind = fromKeyframe->keyKinds()[i];
            neededOriginProperty = false;

            if (property.keyKind() == CSSStyleValuePair::KeyKind::Unknown &&
                keyKind != CSSStyleValuePair::KeyKind::Unknown) {
                neededOriginProperty = true;
            }

            size_t layerSize = 1;
            if (isAnimatableBackgroundProperty(keyKind)) {
                layerSize = m_style->backgroundLayerSize();
            }

            GCVector<GCVector<AnimatedValue*>> values;
            values.resize(layerSize);
            bool isAvailable = true;
            for (size_t l = 0; l < layerSize; l++) {
                Optional<AnimatedValue*> maybeAnimatedValue =
                    AnimatedValue::create(m_style, m_element, property, keyKind,
                                          l, neededOriginProperty);
                if (!maybeAnimatedValue) {
                    isAvailable = false;
                    break;
                }
                AnimatedValue* animatedValue = maybeAnimatedValue.value();
                resolveLengthAnimatedValueIfNeeded(animatedValue, font,
                                                   curFontSize, rootFontSize,
                                                   root, windowSize);
                values[l].push_back(animatedValue);
            }
            if (isAvailable == false) {
                continue;
            }

            GCAtomicVector<double> offsets;
            GCVector<TimingFunction*> timingFunctions;
            offsets.push_back(fromKeyframe->keyframeSelector());
            timingFunctions.push_back(fromKeyframe->timingFunction());

            for (size_t k = 1; k < keyframeSize; k++) {
                auto keyframe = keyframes.keyframe(k);

                property = keyframe->properties()[i];
                if (keyKind != keyframe->keyKinds()[i]) {
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
                if ((keyframeSize - 1 != k) &&
                    property.valueKind() ==
                        CSSStyleValuePair::ValueKind::None) {
                    continue;
                }

                isAvailable = true;
                for (size_t l = 0; l < layerSize; l++) {
                    neededOriginProperty = false;
                    if ((keyframeSize - 1 == k) &&
                        (property.keyKind() ==
                             CSSStyleValuePair::KeyKind::Unknown &&
                         keyKind != CSSStyleValuePair::KeyKind::Unknown)) {
                        neededOriginProperty = true;
                    }
                    Optional<AnimatedValue*> maybeAnimatedValue =
                        AnimatedValue::create(m_style, m_element, property,
                                              keyKind, l, neededOriginProperty);
                    if (!maybeAnimatedValue) {
                        isAvailable = false;
                        break;
                    }
                    AnimatedValue* animatedValue = maybeAnimatedValue.value();
                    resolveLengthAnimatedValueIfNeeded(
                        animatedValue, font, curFontSize, rootFontSize, root,
                        windowSize);
                    values[l].push_back(animatedValue);
                }
                if (isAvailable == false) {
                    continue;
                }

                offsets.push_back(keyframe->keyframeSelector());
                timingFunctions.push_back(keyframe->timingFunction());
            }

            bool gotAnimation =
                applyProperty(s, name, keyKind, values, layerSize, offsets,
                              timingFunctions, duration, delay, iterationCount,
                              direction, playState, fillMode);

            if (gotAnimation == true) {
                // TODO reduce animation duration here with
                // canceledAnimationProgress

                // FIXME: During the loop, gotAnimation may be either true or
                // false, but even a single opportunity for gotAnimation to
                // become true is enough for ret to become True as well. This is
                // quite ambiguous.
                ret = true;
            }
        }

        if (ret == true) {
            m_executor->fireAnimationStartEvent(m_element, name, delay);
        }
    }
    return ret;
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
