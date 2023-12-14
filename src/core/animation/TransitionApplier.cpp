/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "core/animation/TransitionApplier.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/animation/AnimationTask.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/style/CalcData.h"
#include "core/animation/util/AnimationUtil.h"

namespace Starfish {

static Length getLengthWidth(Frame* frame, Element* element, Length length)
{
    if (length.isCalc()) {
        return Length(Length::Fixed,
                      length.calcData()->specifiedValue(
                          containingBlock(frame)->contentWidth(), element));
    }
    return length;
}

static Length getLengthHeight(Frame* frame, Element* element, Length length)
{
    if (length.isCalc()) {
        return Length(Length::Fixed,
                      length.calcData()->specifiedValue(
                          containingBlock(frame)->contentHeight(), element));
    }
    return length;
}

TransitionApplier::TransitionApplier(Element* element, ComputedStyle* oldStyle,
                                     Frame* oldFrame, ComputedStyle* newStyle,
                                     const bool* damagedKeys)
    : m_element(element)
    , m_oldStyle(oldStyle)
    , m_oldFrame(oldFrame)
    , m_newStyle(newStyle)
    , m_damagedKeys(damagedKeys)
    , m_executor(element->document()->animationExecutor())
    , m_gotTransition(false)
{
}

bool TransitionApplier::apply()
{
    bool ret = false;
    StyleTransitionData* data = m_newStyle->transition();

    if (!data) {
        return false;
    }

    for (size_t i = 0; i < data->size(); i++) {
        double duration = data->duration(i).toTimeValue();
        if (duration == 0.0) {
            // No duration.
            continue;
        }

        CSSStyleValuePair::KeyKind property = data->property(i);
        double delay = data->delay(i).toTimeValue();
        TimingFunction* timingFunction = data->timingFunction(i);
        if (property == CSSStyleValuePair::All) {
            applyAll(duration, delay, timingFunction);
        } else {
            applyProperty(property, duration, delay, timingFunction);
        }
    }

    if (m_gotTransition) {
        // TODO reduce animation duration here with
        // canceledAnimationProgress
        ret = true;
    }

    return ret;
}

bool TransitionApplier::canRegisterTransition(
    CSSStyleValuePair::KeyKind property)
{
    return m_damagedKeys[property] &&
           !m_executor->hasActiveTransition(m_element, property);
}

void TransitionApplier::applyProperty(CSSStyleValuePair::KeyKind property,
                                      double duration, double delay,
                                      TimingFunction* timingFunction)
{
    if (AnimationUtil::checkCSSProperty(property, CSSStyleValuePair::Opacity)) {
        applyOpacity(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Transform)) {
        applyTransform(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BackgroundColor,
                   CSSStyleValuePair::Background)) {
        applyBackgroundColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BorderBottomColor,
                   CSSStyleValuePair::BorderColor,
                   CSSStyleValuePair::BorderBottom)) {
        applyBorderBottomColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BorderLeftColor,
                   CSSStyleValuePair::BorderColor,
                   CSSStyleValuePair::BorderLeft)) {
        applyBorderLeftColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BorderRightColor,
                   CSSStyleValuePair::BorderColor,
                   CSSStyleValuePair::BorderRight)) {
        applyBorderRightColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BorderTopColor,
                   CSSStyleValuePair::BorderColor,
                   CSSStyleValuePair::BorderTop)) {
        applyBorderTopColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Color)) {
        applyColor(duration, delay, timingFunction);

    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::CaretColor)) {
        applyCaretColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::OutlineColor)) {
        applyOutlineColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::TextDecorationColor,
                   CSSStyleValuePair::TextDecoration)) {
        applyTextDecorationColor(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Width)) {
        applyWidth(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Height)) {
        applyHeight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MinWidth)) {
        applyMinWidth(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MinHeight)) {
        applyMinHeight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MaxWidth)) {
        applyMaxWidth(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MaxHeight)) {
        applyMaxHeight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MarginTop,
                                               CSSStyleValuePair::Margin)) {
        applyMarginTop(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MarginRight,
                                               CSSStyleValuePair::Margin)) {
        applyMarginRight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MarginBottom,
                                               CSSStyleValuePair::Margin)) {
        applyMarginBottom(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::MarginLeft,
                                               CSSStyleValuePair::Margin)) {
        applyMarginLeft(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::BorderTop,
                                               CSSStyleValuePair::Border)) {
        applyBorderTop(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::BorderRight,
                                               CSSStyleValuePair::Border)) {
        applyBorderRight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::BorderBottom,
                                               CSSStyleValuePair::Border)) {
        applyBorderBottom(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::BorderLeft,
                                               CSSStyleValuePair::Border)) {
        applyBorderLeft(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::PaddingTop,
                                               CSSStyleValuePair::Padding)) {
        applyPaddingTop(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::PaddingRight,
                                               CSSStyleValuePair::Padding)) {
        applyPaddingRight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::PaddingBottom,
                                               CSSStyleValuePair::Padding)) {
        applyPaddingBottom(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::PaddingLeft,
                                               CSSStyleValuePair::Padding)) {
        applyPaddingLeft(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Left)) {
        applyLeft(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Right)) {
        applyRight(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Top)) {
        applyTop(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Bottom)) {
        applyBottom(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BackgroundPositionX,
                   CSSStyleValuePair::BackgroundPosition,
                   CSSStyleValuePair::Background)) {
        applyBackgroundPositionX(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BackgroundPositionY,
                   CSSStyleValuePair::BackgroundPosition,
                   CSSStyleValuePair::Background)) {
        applyBackgroundPositionY(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::BackgroundSize,
                   CSSStyleValuePair::Background)) {
        // NOTE background-size should come after background-position
        applyBackgroundSize(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::FontSize,
                                               CSSStyleValuePair::Font)) {
        applyFontSize(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(property,
                                               CSSStyleValuePair::Visibility)) {
        applyVisibility(duration, delay, timingFunction);
    }
}

void TransitionApplier::applyAll(double duration, double delay,
                                 TimingFunction* timingFunction)
{
    static const std::vector<CSSStyleValuePair::KeyKind> allProperties = {
        CSSStyleValuePair::Opacity,
        CSSStyleValuePair::Transform,
        CSSStyleValuePair::BackgroundColor,
        CSSStyleValuePair::BorderBottomColor,
        CSSStyleValuePair::BorderLeftColor,
        CSSStyleValuePair::BorderRightColor,
        CSSStyleValuePair::BorderTopColor,
        CSSStyleValuePair::Color,
        CSSStyleValuePair::CaretColor,
        CSSStyleValuePair::OutlineColor,
        CSSStyleValuePair::TextDecorationColor,
        CSSStyleValuePair::Width,
        CSSStyleValuePair::Height,
        CSSStyleValuePair::MinWidth,
        CSSStyleValuePair::MinHeight,
        CSSStyleValuePair::MaxWidth,
        CSSStyleValuePair::MaxHeight,
        CSSStyleValuePair::MarginTop,
        CSSStyleValuePair::MarginRight,
        CSSStyleValuePair::MarginBottom,
        CSSStyleValuePair::MarginLeft,
        CSSStyleValuePair::BorderTop,
        CSSStyleValuePair::BorderRight,
        CSSStyleValuePair::BorderBottom,
        CSSStyleValuePair::BorderLeft,
        CSSStyleValuePair::PaddingTop,
        CSSStyleValuePair::PaddingRight,
        CSSStyleValuePair::PaddingBottom,
        CSSStyleValuePair::PaddingLeft,
        CSSStyleValuePair::Left,
        CSSStyleValuePair::Right,
        CSSStyleValuePair::Top,
        CSSStyleValuePair::Bottom,
        CSSStyleValuePair::BackgroundPositionX,
        CSSStyleValuePair::BackgroundPositionY,
        CSSStyleValuePair::BackgroundSize,
        CSSStyleValuePair::FontSize,
        CSSStyleValuePair::Visibility,
    };

    for (auto keyKind : allProperties) {
        applyProperty(keyKind, duration, delay, timingFunction);
    }
}

void TransitionApplier::applyOpacity(double duration, double delay,
                                     TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Opacity)) {
        return;
    }
    auto task = new ActiveOpacityAnimationTask(
        m_element, CSSStyleValuePair::Opacity,
        AnimatedValue(m_oldStyle->opacity()),
        AnimatedValue(m_newStyle->opacity()), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyTransform(double duration, double delay,
                                       TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Transform)) {
        return;
    }
    if (m_oldFrame && m_oldFrame->isTransformable()) {
        auto newTransform =
            m_newStyle->rareComputedStyleData()->ensureTransforms();
        auto task = new ActiveTransformAnimationTask(
            m_element, CSSStyleValuePair::Transform,
            AnimatedValue(
                m_oldStyle->rareComputedStyleData()->ensureTransforms()),
            AnimatedValue(newTransform), duration, delay, timingFunction,
            newTransform);
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBackgroundColor(double duration, double delay,
                                             TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BackgroundColor)) {
        return;
    }
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::BackgroundColor,
        AnimatedValue(m_oldStyle->backgroundColor()),
        AnimatedValue(m_newStyle->backgroundColor()), duration, delay,
        timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyBorderBottomColor(double duration, double delay,
                                               TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderBottomColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->border().bottom().color();
    Unit::Color newColor = m_newStyle->border().bottom().color();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::BorderBottomColor,
        AnimatedValue(oldColor), AnimatedValue(newColor), duration, delay,
        timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyBorderLeftColor(double duration, double delay,
                                             TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderLeftColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->border().left().color();
    Unit::Color newColor = m_newStyle->border().left().color();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::BorderLeftColor, AnimatedValue(oldColor),
        AnimatedValue(newColor), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyBorderRightColor(double duration, double delay,
                                              TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderRightColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->border().right().color();
    Unit::Color newColor = m_newStyle->border().right().color();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::BorderRightColor, AnimatedValue(oldColor),
        AnimatedValue(newColor), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyBorderTopColor(double duration, double delay,
                                            TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderTopColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->border().top().color();
    Unit::Color newColor = m_newStyle->border().top().color();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::BorderTopColor, AnimatedValue(oldColor),
        AnimatedValue(newColor), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyColor(double duration, double delay,
                                   TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Color)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->color();
    Unit::Color newColor = m_newStyle->color();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::Color, AnimatedValue(oldColor),
        AnimatedValue(newColor), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyCaretColor(double duration, double delay,
                                        TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::CaretColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->caretColor();
    Unit::Color newColor = m_newStyle->caretColor();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::CaretColor, AnimatedValue(oldColor),
        AnimatedValue(newColor), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyOutlineColor(double duration, double delay,
                                          TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::OutlineColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->outlineColor();
    Unit::Color newColor = m_newStyle->outlineColor();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::OutlineColor, AnimatedValue(oldColor),
        AnimatedValue(newColor), duration, delay, timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyTextDecorationColor(double duration, double delay,
                                                 TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::TextDecorationColor)) {
        return;
    }
    Unit::Color oldColor = m_oldStyle->textDecorationColor();
    Unit::Color newColor = m_newStyle->textDecorationColor();
    auto task = new ActiveColorAnimationTask(
        m_element, CSSStyleValuePair::TextDecorationColor,
        AnimatedValue(oldColor), AnimatedValue(newColor), duration, delay,
        timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyWidth(double duration, double delay,
                                   TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Width)) {
        return;
    }
    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        auto oldWidth =
            getLengthWidth(m_oldFrame, m_element, m_oldStyle->width());
        auto newWidth =
            getLengthWidth(m_oldFrame, m_element, m_newStyle->width());

        if ((oldWidth.isPercent() || oldWidth.isFixed()) &&
            (newWidth.isPercent() || newWidth.isFixed())) {
            auto fromValue = oldWidth;
            auto toValue = newWidth;

            if (toValue.isPercent() && !fromValue.isPercent()) {
                FrameBox* cb = containingBlock(m_oldFrame);
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(Length::Percent,
                                       m_oldFrame->asFrameBox()->width() /
                                           cb->contentWidth());
                } else {
                    fromValue =
                        Length(Length::Percent,
                               m_oldFrame->asFrameBox()->contentWidth() /
                                   cb->contentWidth());
                }
            } else if (toValue.isFixed() && fromValue.isFixed() == false) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = m_oldFrame->asFrameBox()->width();
                } else {
                    value = m_oldFrame->asFrameBox()->contentWidth();
                }
                fromValue = Length(Length::Fixed, value);
            }

            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::Width, AnimatedValue(fromValue),
                AnimatedValue(toValue), duration, delay, timingFunction,
                m_newStyle->width());
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyHeight(double duration, double delay,
                                    TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Height)) {
        return;
    }
    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        auto oldHeight =
            getLengthHeight(m_oldFrame, m_element, m_oldStyle->height());
        auto newHeight =
            getLengthHeight(m_oldFrame, m_element, m_newStyle->height());

        if ((oldHeight.isPercent() || oldHeight.isFixed()) &&
            (newHeight.isPercent() || newHeight.isFixed())) {
            auto fromValue = oldHeight;
            auto toValue = newHeight;

            if (toValue.isPercent() && fromValue.isPercent() == false) {
                FrameBox* cb = containingBlock(m_oldFrame);
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(Length::Percent,
                                       m_oldFrame->asFrameBox()->height() /
                                           cb->contentHeight());
                } else {
                    fromValue =
                        Length(Length::Percent,
                               m_oldFrame->asFrameBox()->contentHeight() /
                                   cb->contentHeight());
                }
            } else if (toValue.isFixed() && fromValue.isFixed() == false) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = m_oldFrame->asFrameBox()->height();
                } else {
                    value = m_oldFrame->asFrameBox()->contentHeight();
                }
                fromValue = Length(Length::Fixed, value);
            }

            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::Height, AnimatedValue(fromValue),
                AnimatedValue(toValue), duration, delay, timingFunction,
                m_newStyle->height());
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyMinWidth(double duration, double delay,
                                      TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MinWidth)) {
        return;
    }
    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        auto oldWidth =
            getLengthWidth(m_oldFrame, m_element, m_oldStyle->minWidth());
        auto newWidth =
            getLengthWidth(m_oldFrame, m_element, m_newStyle->minWidth());

        if ((oldWidth.isPercent() || oldWidth.isFixed()) &&
            (newWidth.isPercent() || newWidth.isFixed())) {
            auto fromValue = oldWidth;
            auto toValue = newWidth;

            if (toValue.isPercent() && fromValue.isPercent() == false) {
                FrameBox* cb = containingBlock(m_oldFrame);
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(Length::Percent,
                                       m_oldFrame->asFrameBox()->width() /
                                           cb->contentWidth());
                } else {
                    fromValue =
                        Length(Length::Percent,
                               m_oldFrame->asFrameBox()->contentWidth() /
                                   cb->contentWidth());
                }
            } else if (toValue.isFixed() && fromValue.isFixed() == false) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = m_oldFrame->asFrameBox()->width();
                } else {
                    value = m_oldFrame->asFrameBox()->contentWidth();
                }
                fromValue = Length(Length::Fixed, value);
            }

            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::MinWidth,
                AnimatedValue(fromValue), AnimatedValue(toValue), duration,
                delay, timingFunction, m_newStyle->minWidth());
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyMinHeight(double duration, double delay,
                                       TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MinHeight)) {
        return;
    }
    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        auto oldHeight =
            getLengthHeight(m_oldFrame, m_element, m_oldStyle->minHeight());
        auto newHeight =
            getLengthHeight(m_oldFrame, m_element, m_newStyle->minHeight());

        if ((oldHeight.isPercent() || oldHeight.isFixed()) &&
            (newHeight.isPercent() || newHeight.isFixed())) {
            auto fromValue = oldHeight;
            auto toValue = newHeight;

            if (toValue.isPercent() && fromValue.isPercent() == false) {
                FrameBox* cb = containingBlock(m_oldFrame);
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(Length::Percent,
                                       m_oldFrame->asFrameBox()->height() /
                                           cb->contentHeight());
                } else {
                    fromValue =
                        Length(Length::Percent,
                               m_oldFrame->asFrameBox()->contentHeight() /
                                   cb->contentHeight());
                }
            } else if (toValue.isFixed() && fromValue.isFixed() == false) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = m_oldFrame->asFrameBox()->height();
                } else {
                    value = m_oldFrame->asFrameBox()->contentHeight();
                }
                fromValue = Length(Length::Fixed, value);
            }

            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::MinHeight,
                AnimatedValue(fromValue), AnimatedValue(toValue), duration,
                delay, timingFunction, m_newStyle->minHeight());
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyMaxWidth(double duration, double delay,
                                      TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MaxWidth)) {
        return;
    }
    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        auto oldWidth =
            getLengthWidth(m_oldFrame, m_element, m_oldStyle->maxWidth());
        auto newWidth =
            getLengthWidth(m_oldFrame, m_element, m_newStyle->maxWidth());

        if ((oldWidth.isPercent() || oldWidth.isFixed()) &&
            (newWidth.isPercent() || newWidth.isFixed())) {
            auto fromValue = oldWidth;
            auto toValue = newWidth;

            if (toValue.isPercent() && fromValue.isPercent() == false) {
                FrameBox* cb = containingBlock(m_oldFrame);
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(Length::Percent,
                                       m_oldFrame->asFrameBox()->width() /
                                           cb->contentWidth());
                } else {
                    fromValue =
                        Length(Length::Percent,
                               m_oldFrame->asFrameBox()->contentWidth() /
                                   cb->contentWidth());
                }
            } else if (toValue.isFixed() && fromValue.isFixed() == false) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = m_oldFrame->asFrameBox()->width();
                } else {
                    value = m_oldFrame->asFrameBox()->contentWidth();
                }
                fromValue = Length(Length::Fixed, value);
            }

            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::MaxWidth,
                AnimatedValue(fromValue), AnimatedValue(toValue), duration,
                delay, timingFunction, m_newStyle->maxWidth());
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyMaxHeight(double duration, double delay,
                                       TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MaxHeight)) {
        return;
    }
    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        auto oldHeight =
            getLengthHeight(m_oldFrame, m_element, m_oldStyle->maxHeight());
        auto newHeight =
            getLengthHeight(m_oldFrame, m_element, m_newStyle->maxHeight());

        if ((oldHeight.isPercent() || oldHeight.isFixed()) &&
            (newHeight.isPercent() || newHeight.isFixed())) {
            auto fromValue = oldHeight;
            auto toValue = newHeight;

            if (toValue.isPercent() && fromValue.isPercent() == false) {
                FrameBox* cb = containingBlock(m_oldFrame);
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(Length::Percent,
                                       m_oldFrame->asFrameBox()->height() /
                                           cb->contentHeight());
                } else {
                    fromValue =
                        Length(Length::Percent,
                               m_oldFrame->asFrameBox()->contentHeight() /
                                   cb->contentHeight());
                }
            } else if (toValue.isFixed() && fromValue.isFixed() == false) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = m_oldFrame->asFrameBox()->height();
                } else {
                    value = m_oldFrame->asFrameBox()->contentHeight();
                }
                fromValue = Length(Length::Fixed, value);
            }

            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::MaxHeight,
                AnimatedValue(fromValue), AnimatedValue(toValue), duration,
                delay, timingFunction, m_newStyle->maxHeight());
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyMarginTop(double duration, double delay,
                                       TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MarginTop)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::marginTopToAnimatedValue(m_oldStyle, m_newStyle,
                                                m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginTop, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->margin().top());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyMarginRight(double duration, double delay,
                                         TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MarginRight)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::marginRightToAnimatedValue(m_oldStyle, m_newStyle,
                                                  m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginRight, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->margin().right());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyMarginBottom(double duration, double delay,
                                          TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MarginBottom)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::marginBottomToAnimatedValue(m_oldStyle, m_newStyle,
                                                   m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginBottom, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->margin().bottom());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyMarginLeft(double duration, double delay,
                                        TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::MarginLeft)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::marginLeftToAnimatedValue(m_oldStyle, m_newStyle,
                                                 m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::MarginLeft, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->margin().left());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBorderTop(double duration, double delay,
                                       TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderTop)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::borderTopToAnimatedValue(m_oldStyle, m_newStyle,
                                                m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderTop, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->border().top().width());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBorderRight(double duration, double delay,
                                         TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderRight)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::borderRightToAnimatedValue(m_oldStyle, m_newStyle,
                                                  m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderRight, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->border().right().width());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBorderBottom(double duration, double delay,
                                          TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderBottom)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::borderBottomToAnimatedValue(m_oldStyle, m_newStyle,
                                                   m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderBottom, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->border().bottom().width());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBorderLeft(double duration, double delay,
                                        TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BorderLeft)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::borderLeftToAnimatedValue(m_oldStyle, m_newStyle,
                                                 m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::BorderLeft, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->border().left().width());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyPaddingTop(double duration, double delay,
                                        TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::PaddingTop)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::paddingTopToAnimatedValue(m_oldStyle, m_newStyle,
                                                 m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingTop, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->padding().top());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyPaddingRight(double duration, double delay,
                                          TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::PaddingRight)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::paddingRightToAnimatedValue(m_oldStyle, m_newStyle,
                                                   m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingRight, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->padding().right());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyPaddingBottom(double duration, double delay,
                                           TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::PaddingBottom)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::paddingBottomToAnimatedValue(m_oldStyle, m_newStyle,
                                                    m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingBottom, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->padding().bottom());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyPaddingLeft(double duration, double delay,
                                         TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::PaddingLeft)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::paddingLeftToAnimatedValue(m_oldStyle, m_newStyle,
                                                  m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::PaddingLeft, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->padding().left());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyLeft(double duration, double delay,
                                  TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Left)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::lengthToAnimatedValue(
            m_oldStyle->left(), m_newStyle->left(), m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Left, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->left());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyRight(double duration, double delay,
                                   TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Right)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::lengthToAnimatedValue(
            m_oldStyle->right(), m_newStyle->right(), m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Right, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->right());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyTop(double duration, double delay,
                                 TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Top)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::lengthToAnimatedValue(
            m_oldStyle->top(), m_newStyle->top(), m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Top, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->top());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBottom(double duration, double delay,
                                    TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Bottom)) {
        return;
    }
    AnimatedValue v1, v2;
    if (AnimationUtil::lengthToAnimatedValue(
            m_oldStyle->bottom(), m_newStyle->bottom(), m_element, v1, v2)) {
        auto task = new ActiveLengthAnimationTask(
            m_element, CSSStyleValuePair::Bottom, AnimatedValue(v1),
            AnimatedValue(v2), duration, delay, timingFunction,
            m_newStyle->bottom());
        m_executor->registerTransition(task, m_newStyle);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyBackgroundPositionX(double duration, double delay,
                                                 TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BackgroundPositionX)) {
        return;
    }
    if (m_oldStyle->hasBlockLikeDisplay() == false ||
        m_newStyle->hasBlockLikeDisplay() == false) {
        // TODO Inline Element
        return;
    }
    FrameBox* oldPaintingBox = m_oldFrame->asFrameBox();
    size_t layerSize = m_newStyle->backgroundLayerSize();
    for (size_t i = 0; i < layerSize; i++) {
        AnimatedValue pos1, pos2;
        if (AnimationUtil::backgroundPosXToAnimatedValue(
                m_oldStyle, m_newStyle, oldPaintingBox, m_element, pos1, pos2,
                i)) {
            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::BackgroundPositionX,
                AnimatedValue(pos1), AnimatedValue(pos2), duration, delay,
                timingFunction, m_newStyle->backgroundPositionX(i), i);
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyBackgroundPositionY(double duration, double delay,
                                                 TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BackgroundPositionY)) {
        return;
    }
    if (m_oldStyle->hasBlockLikeDisplay() == false ||
        m_newStyle->hasBlockLikeDisplay() == false) {
        // TODO Inline Element
        return;
    }
    FrameBox* oldPaintingBox = m_oldFrame->asFrameBox();
    size_t layerSize = m_newStyle->backgroundLayerSize();
    for (size_t i = 0; i < layerSize; i++) {
        AnimatedValue pos1, pos2;
        if (AnimationUtil::backgroundPosYToAnimatedValue(
                m_oldStyle, m_newStyle, oldPaintingBox, m_element, pos1, pos2,
                i)) {
            auto task = new ActiveLengthAnimationTask(
                m_element, CSSStyleValuePair::BackgroundPositionY,
                AnimatedValue(pos1), AnimatedValue(pos2), duration, delay,
                timingFunction, m_newStyle->backgroundPositionY(i), i);
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyBackgroundSize(double duration, double delay,
                                            TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::BackgroundSize)) {
        return;
    }
    if (m_oldStyle->hasBlockLikeDisplay() == false ||
        m_newStyle->hasBlockLikeDisplay() == false) {
        // TODO Inline Element
        return;
    }
    FrameBox* oldPaintingBox = m_oldFrame->asFrameBox();
    size_t layerSize = m_newStyle->backgroundLayerSize();
    for (size_t i = 0; i < layerSize; i++) {
        AnimatedValue size1, size2;
        if (AnimationUtil::backgroundSizeToAnimatedValue(
                m_oldStyle, m_newStyle, oldPaintingBox, m_element, size1, size2,
                i)) {
            auto task = new ActiveLengthSizeAnimationTask(
                m_element, CSSStyleValuePair::BackgroundSize,
                AnimatedValue(size1), AnimatedValue(size2), duration, delay,
                timingFunction, m_newStyle->backgroundSizeLengthValue(i), i);
            m_executor->registerTransition(task, m_newStyle);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyFontSize(double duration, double delay,
                                      TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::FontSize)) {
        return;
    }
    auto task = new ActiveLengthAnimationTask(
        m_element, CSSStyleValuePair::FontSize,
        AnimatedValue(Length(Length::Fixed, m_oldStyle->fixedFontSize())),
        AnimatedValue(Length(Length::Fixed, m_newStyle->fixedFontSize())),
        duration, delay, timingFunction,
        Length(Length::Fixed, m_newStyle->fixedFontSize()));
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

void TransitionApplier::applyVisibility(double duration, double delay,
                                        TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::Visibility)) {
        return;
    }
    auto task = new ActiveVisibilityAnimationTask(
        m_element, CSSStyleValuePair::Visibility,
        AnimatedValue(m_oldStyle->visibility()),
        AnimatedValue(m_newStyle->visibility()), duration, delay,
        timingFunction);
    m_executor->registerTransition(task, m_newStyle);
    m_gotTransition = true;
}

} // namespace Starfish
