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
#include "core/animation/AnimationExecutor.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/animation/AnimationTask.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/style/CalcData.h"
#include "core/animation/util/AnimationUtil.h"

namespace Starfish {

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
    if (property == CSSStyleValuePair::KeyKind::Opacity) {
        applyOpacity(duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Transform) {
        applyTransform(duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BackgroundColor,
                   CSSStyleValuePair::KeyKind::Background)) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BackgroundColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->backgroundColor();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderBottomColor,
                   CSSStyleValuePair::KeyKind::BorderColor,
                   CSSStyleValuePair::KeyKind::BorderBottom)) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderBottomColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().bottom().color();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderLeftColor,
                   CSSStyleValuePair::KeyKind::BorderColor,
                   CSSStyleValuePair::KeyKind::BorderLeft)) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderLeftColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().left().color();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderRightColor,
                   CSSStyleValuePair::KeyKind::BorderColor,
                   CSSStyleValuePair::KeyKind::BorderRight)) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderRightColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().right().color();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderTopColor,
                   CSSStyleValuePair::KeyKind::BorderColor,
                   CSSStyleValuePair::KeyKind::BorderTop)) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderTopColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().top().color();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Color) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::Color,
            [](ComputedStyle* style) -> Unit::Color { return style->color(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::CaretColor) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::CaretColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->caretColor();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::OutlineColor) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::OutlineColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->outlineColor();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::TextDecorationColor,
                   CSSStyleValuePair::KeyKind::TextDecoration)) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::TextDecorationColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->textDecorationColor();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Width) {
        applyActiveLengthAnimationTaskForFrameBoxSize(
            CSSStyleValuePair::KeyKind::Width,
            [](ComputedStyle* style) -> Length { return style->width(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->width(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->contentWidth(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Height) {
        applyActiveLengthAnimationTaskForFrameBoxSize(
            CSSStyleValuePair::KeyKind::Height,
            [](ComputedStyle* style) -> Length { return style->height(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->height(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->contentHeight(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MinWidth) {
        applyActiveLengthAnimationTaskForFrameBoxSize(
            CSSStyleValuePair::KeyKind::MinWidth,
            [](ComputedStyle* style) -> Length { return style->minWidth(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->width(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->contentWidth(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MinHeight) {
        applyActiveLengthAnimationTaskForFrameBoxSize(
            CSSStyleValuePair::KeyKind::MinHeight,
            [](ComputedStyle* style) -> Length { return style->minHeight(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->height(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->contentHeight(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MaxWidth) {
        applyActiveLengthAnimationTaskForFrameBoxSize(
            CSSStyleValuePair::KeyKind::MaxWidth,
            [](ComputedStyle* style) -> Length { return style->maxWidth(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->width(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->contentWidth(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MaxHeight) {
        applyActiveLengthAnimationTaskForFrameBoxSize(
            CSSStyleValuePair::KeyKind::MaxHeight,
            [](ComputedStyle* style) -> Length { return style->maxHeight(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->height(); },
            [](FrameBox* fb) -> LayoutUnit { return fb->contentHeight(); },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::MarginTop,
                   CSSStyleValuePair::KeyKind::Margin)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginTop,
            [](ComputedStyle* style) -> Length {
                return style->margin().top();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::MarginRight,
                   CSSStyleValuePair::KeyKind::Margin)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginRight,
            [](ComputedStyle* style) -> Length {
                return style->margin().right();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::MarginBottom,
                   CSSStyleValuePair::KeyKind::Margin)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginBottom,
            [](ComputedStyle* style) -> Length {
                return style->margin().bottom();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::MarginLeft,
                   CSSStyleValuePair::KeyKind::Margin)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginLeft,
            [](ComputedStyle* style) -> Length {
                return style->margin().left();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderTop,
                   CSSStyleValuePair::KeyKind::Border)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderTop,
            [](ComputedStyle* style) -> Length {
                return style->border().top().width();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderRight,
                   CSSStyleValuePair::KeyKind::Border)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderRight,
            [](ComputedStyle* style) -> Length {
                return style->border().right().width();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderBottom,
                   CSSStyleValuePair::KeyKind::Border)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderBottom,
            [](ComputedStyle* style) -> Length {
                return style->border().bottom().width();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::BorderLeft,
                   CSSStyleValuePair::KeyKind::Border)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderLeft,
            [](ComputedStyle* style) -> Length {
                return style->border().left().width();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::PaddingTop,
                   CSSStyleValuePair::KeyKind::Padding)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingTop,
            [](ComputedStyle* style) -> Length {
                return style->padding().top();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::PaddingRight,
                   CSSStyleValuePair::KeyKind::Padding)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingRight,
            [](ComputedStyle* style) -> Length {
                return style->padding().right();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::PaddingBottom,
                   CSSStyleValuePair::KeyKind::Padding)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingBottom,
            [](ComputedStyle* style) -> Length {
                return style->padding().bottom();
            },
            duration, delay, timingFunction);
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::PaddingLeft,
                   CSSStyleValuePair::KeyKind::Padding)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingLeft,
            [](ComputedStyle* style) -> Length {
                return style->padding().left();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Left) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::Left,
            [](ComputedStyle* style) -> Length { return style->left(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Right) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::Right,
            [](ComputedStyle* style) -> Length { return style->right(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Top) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::Top,
            [](ComputedStyle* style) -> Length { return style->top(); },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Bottom) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::Bottom,
            [](ComputedStyle* style) -> Length { return style->bottom(); },
            duration, delay, timingFunction);
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
    } else if (AnimationUtil::checkCSSProperty(
                   property, CSSStyleValuePair::KeyKind::FontSize,
                   CSSStyleValuePair::KeyKind::Font)) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::FontSize,
            [](ComputedStyle* style) -> Length {
                return Length(Length::Fixed, style->fixedFontSize());
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::Visibility) {
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
    m_executor->registerTransition(task);
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
        m_executor->registerTransition(task);
        m_gotTransition = true;
    }
}

void TransitionApplier::applyActiveColorAnimationTask(
    CSSStyleValuePair::KeyKind keyKind,
    const std::function<Unit::Color(ComputedStyle*)>& colorValueGetter,
    double duration, double delay, TimingFunction* timingFunction)
{
    if (!canRegisterTransition(keyKind)) {
        return;
    }

    AnimatedValue from = colorValueGetter(m_oldStyle);
    AnimatedValue to = colorValueGetter(m_newStyle);
    auto task = new ActiveColorAnimationTask(m_element, keyKind, from, to,
                                             duration, delay, timingFunction);
    m_executor->registerTransition(task);
    m_gotTransition = true;
}

void TransitionApplier::applyActiveLengthAnimationTaskForFrameBoxSize(
    CSSStyleValuePair::KeyKind keyKind,
    const std::function<Length(ComputedStyle*)>& lengthValueGetter,
    const std::function<LayoutUnit(FrameBox*)>& frameBoxSizeValueGetter,
    const std::function<LayoutUnit(FrameBox*)>& contentSizeValueGetter,
    double duration, double delay, TimingFunction* timingFunction)
{
    if (!canRegisterTransition(keyKind)) {
        return;
    }

    if (m_oldFrame != nullptr && m_oldFrame->isFrameBox()) {
        FrameBox* cb = containingBlock(m_oldFrame);
        Length oldLength = lengthValueGetter(m_oldStyle);
        if (oldLength.isCalc()) {
            oldLength = Length(Length::Fixed,
                               oldLength.calcData()->specifiedValue(
                                   contentSizeValueGetter(cb), m_element));
        }

        Length newLength = lengthValueGetter(m_newStyle);
        if (newLength.isCalc()) {
            newLength = Length(Length::Fixed,
                               newLength.calcData()->specifiedValue(
                                   contentSizeValueGetter(cb), m_element));
        }

        if ((oldLength.isPercent() || oldLength.isFixed()) &&
            (newLength.isPercent() || newLength.isFixed())) {
            Length fromValue = oldLength;
            Length toValue = newLength;

            if (toValue.isPercent() && !fromValue.isPercent()) {
                if (cb->contentWidth() == 0) {
                    fromValue = Length(Length::Percent, 0);
                } else if (m_oldStyle->boxSizing() ==
                           BoxSizingValue::BorderBoxBoxSizingValue) {
                    fromValue = Length(
                        Length::Percent,
                        frameBoxSizeValueGetter(m_oldFrame->asFrameBox()) /
                            contentSizeValueGetter(cb));
                } else {
                    fromValue = Length(
                        Length::Percent,
                        contentSizeValueGetter(m_oldFrame->asFrameBox()) /
                            contentSizeValueGetter(cb));
                }
            } else if (toValue.isFixed() && !fromValue.isFixed()) {
                float value;
                if (m_oldStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = frameBoxSizeValueGetter(m_oldFrame->asFrameBox());
                } else {
                    value = contentSizeValueGetter(m_oldFrame->asFrameBox());
                }
                fromValue = Length(Length::Fixed, value);
            }

            AnimatedValue from = fromValue;
            AnimatedValue to = toValue;
            auto task = new ActiveLengthAnimationTask(
                m_element, keyKind, from, to, duration, delay, timingFunction,
                lengthValueGetter(m_newStyle));
            m_executor->registerTransition(task);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyActiveLengthAnimationTask(
    CSSStyleValuePair::KeyKind keyKind,
    const std::function<Length(ComputedStyle*)>& lengthValueGetter,
    double duration, double delay, TimingFunction* timingFunction)
{
    if (!canRegisterTransition(keyKind)) {
        return;
    }

    if (keyKind == CSSStyleValuePair::KeyKind::MarginTop ||
        keyKind == CSSStyleValuePair::KeyKind::MarginBottom) {
        // NOTE: This originated from legacy code.
        // Vertical margins will not have any effect on non-replaced inline
        // elements.
        if (m_oldStyle->display() == DisplayValue::InlineDisplayValue ||
            m_newStyle->display() == DisplayValue::InlineDisplayValue) {
            return;
        }
    }

    AnimatedValue from, to;
    if (!AnimationUtil::lengthToAnimatedValue(lengthValueGetter(m_oldStyle),
                                              lengthValueGetter(m_newStyle),
                                              m_element, from, to)) {
        return;
    }

    auto task = new ActiveLengthAnimationTask(m_element, keyKind, from, to,
                                              duration, delay, timingFunction,
                                              lengthValueGetter(m_newStyle));
    m_executor->registerTransition(task);
    m_gotTransition = true;
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
            m_executor->registerTransition(task);
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
            m_executor->registerTransition(task);
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
            m_executor->registerTransition(task);
            m_gotTransition = true;
        }
    }
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
    m_executor->registerTransition(task);
    m_gotTransition = true;
}

} // namespace Starfish
