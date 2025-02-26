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
#include "core/style/CSSProperty.h"

namespace Starfish {

static void fillActiveAnimationTaskInitForTransition(
    Element* element, CSSStyleValuePair::KeyKind property, double duration,
    double delay, TimingFunction* timingFunction, ActiveAnimationTaskInit& init)
{
    init.animationType = AnimationType::Transition;
    init.target = element;
    init.targetProperty = property;
    init.playState = AnimationPlayStateValue::Running;
    init.fillMode = AnimationFillModeValue::None;
    init.durationInMs = duration;
    init.delayInMs = delay;
    init.iterationCount = 0;
    init.timingFunctions.push_back(timingFunction);
}

TransitionApplier::TransitionApplier(Element* element, ComputedStyle* fromStyle,
                                     Optional<Frame*> oldFrame,
                                     ComputedStyle* toStyle,
                                     const bool* damagedKeys)
    : m_element(element)
    , m_fromStyle(fromStyle)
    , m_oldFrame(oldFrame)
    , m_toStyle(toStyle)
    , m_damagedKeys(damagedKeys)
    , m_executor(element->document()->animationExecutor())
    , m_gotTransition(false)
{
}

bool TransitionApplier::apply()
{
    bool ret = false;
    StyleTransitionData* data = m_toStyle->transition();

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

void TransitionApplier::applyShorthandProperty(
    CSSStyleValuePair::KeyKind property, double duration, double delay,
    TimingFunction* timingFunction)
{
    // std::pair<shorthands, longhands>
    auto constituentPropertiesPair =
        CSSPropertyHelper::decomposeIntoConstituentAnimatableProperties(
            property);

    // Apply shorthands
    for (auto& shorthand : constituentPropertiesPair.first) {
        // Shorthand properties can be composed of other shorthand properties.
        // Make recursive calls on them.
        applyShorthandProperty(shorthand, duration, delay, timingFunction);
    }

    // Apply longhands
    for (auto& longhand : constituentPropertiesPair.second) {
        applyLonghandProperty(longhand, duration, delay, timingFunction);
    }
}

void TransitionApplier::applyLonghandProperty(
    CSSStyleValuePair::KeyKind property, double duration, double delay,
    TimingFunction* timingFunction)
{
    if (property == CSSStyleValuePair::KeyKind::Opacity) {
        applyOpacity(duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::Transform) {
        applyTransform(duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BackgroundColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->backgroundColor();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderBottomColor) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderBottomColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().bottom().color();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderLeftColor) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderLeftColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().left().color();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderRightColor) {
        applyActiveColorAnimationTask(
            CSSStyleValuePair::KeyKind::BorderRightColor,
            [](ComputedStyle* style) -> Unit::Color {
                return style->border().right().color();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderTopColor) {
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
    } else if (property == CSSStyleValuePair::KeyKind::TextDecorationColor) {
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
    } else if (property == CSSStyleValuePair::KeyKind::MarginTop) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginTop,
            [](ComputedStyle* style) -> Length {
                return style->margin().top();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MarginRight) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginRight,
            [](ComputedStyle* style) -> Length {
                return style->margin().right();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MarginBottom) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginBottom,
            [](ComputedStyle* style) -> Length {
                return style->margin().bottom();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::MarginLeft) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::MarginLeft,
            [](ComputedStyle* style) -> Length {
                return style->margin().left();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderTopWidth) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderTopWidth,
            [](ComputedStyle* style) -> Length {
                return style->border().top().width();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderRightWidth) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderRightWidth,
            [](ComputedStyle* style) -> Length {
                return style->border().right().width();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderBottomWidth) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderBottomWidth,
            [](ComputedStyle* style) -> Length {
                return style->border().bottom().width();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::BorderLeftWidth) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::BorderLeftWidth,
            [](ComputedStyle* style) -> Length {
                return style->border().left().width();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::PaddingTop) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingTop,
            [](ComputedStyle* style) -> Length {
                return style->padding().top();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::PaddingRight) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingRight,
            [](ComputedStyle* style) -> Length {
                return style->padding().right();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::PaddingBottom) {
        applyActiveLengthAnimationTask(
            CSSStyleValuePair::KeyKind::PaddingBottom,
            [](ComputedStyle* style) -> Length {
                return style->padding().bottom();
            },
            duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::PaddingLeft) {
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
    } else if (property == CSSStyleValuePair::BackgroundPositionX) {
        applyBackgroundPositionX(duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::BackgroundPositionY) {
        applyBackgroundPositionY(duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::BackgroundSize) {
        // NOTE background-size should come after background-position
        applyBackgroundSize(duration, delay, timingFunction);
    } else if (property == CSSStyleValuePair::KeyKind::FontSize) {
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

void TransitionApplier::applyProperty(CSSStyleValuePair::KeyKind property,
                                      double duration, double delay,
                                      TimingFunction* timingFunction)
{
    if (CSSPropertyHelper::isAnimatableShorthandProperty(property)) {
        applyShorthandProperty(property, duration, delay, timingFunction);
    } else if (CSSPropertyHelper::isAnimatableLonghandProperty(property)) {
        applyLonghandProperty(property, duration, delay, timingFunction);
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

    ActiveAnimationTaskInit init;
    fillActiveAnimationTaskInitForTransition(
        m_element, CSSStyleValuePair::KeyKind::Opacity, duration, delay,
        timingFunction, init);
    init.animatedValues.push_back(new AnimatedValue(m_fromStyle->opacity()));
    init.animatedValues.push_back(new AnimatedValue(m_toStyle->opacity()));

    auto task = new ActiveOpacityAnimationTask(init);
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
            m_toStyle->rareComputedStyleData()->ensureTransforms();
        ActiveAnimationTaskInit init;
        fillActiveAnimationTaskInitForTransition(
            m_element, CSSStyleValuePair::KeyKind::Transform, duration, delay,
            timingFunction, init);
        init.animatedValues.push_back(new AnimatedValue(
            m_fromStyle->rareComputedStyleData()->ensureTransforms()));
        init.animatedValues.push_back(new AnimatedValue(newTransform));

        auto task = new ActiveTransformAnimationTask(init, newTransform);
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

    ActiveAnimationTaskInit init;
    fillActiveAnimationTaskInitForTransition(m_element, keyKind, duration,
                                             delay, timingFunction, init);
    init.animatedValues.push_back(
        new AnimatedValue(colorValueGetter(m_fromStyle)));
    init.animatedValues.push_back(
        new AnimatedValue(colorValueGetter(m_toStyle)));

    auto task = new ActiveColorAnimationTask(init);
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

    if (m_oldFrame.hasValue() && m_oldFrame->isFrameBox()) {
        FrameBox* cb = containingBlock(m_oldFrame.getValue());
        Length oldLength = lengthValueGetter(m_fromStyle);
        if (oldLength.isCalc()) {
            oldLength = Length(Length::Fixed,
                               oldLength.calcData()->specifiedValue(
                                   contentSizeValueGetter(cb), m_element));
        }

        Length newLength = lengthValueGetter(m_toStyle);
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
                } else if (m_fromStyle->boxSizing() ==
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
                if (m_fromStyle->boxSizing() ==
                    BoxSizingValue::BorderBoxBoxSizingValue) {
                    value = frameBoxSizeValueGetter(m_oldFrame->asFrameBox());
                } else {
                    value = contentSizeValueGetter(m_oldFrame->asFrameBox());
                }
                fromValue = Length(Length::Fixed, value);
            }

            ActiveAnimationTaskInit init;
            fillActiveAnimationTaskInitForTransition(
                m_element, keyKind, duration, delay, timingFunction, init);
            init.animatedValues.push_back(new AnimatedValue(fromValue));
            init.animatedValues.push_back(new AnimatedValue(toValue));

            auto task = new ActiveLengthAnimationTask(
                init, lengthValueGetter(m_toStyle));
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
        if (m_fromStyle->display() == DisplayValue::InlineDisplayValue ||
            m_toStyle->display() == DisplayValue::InlineDisplayValue) {
            return;
        }
    }

    AnimatedValue from, to;
    if (!AnimationUtil::lengthToAnimatedValue(lengthValueGetter(m_fromStyle),
                                              lengthValueGetter(m_toStyle),
                                              m_element, from, to)) {
        return;
    }

    ActiveAnimationTaskInit init;
    fillActiveAnimationTaskInitForTransition(m_element, keyKind, duration,
                                             delay, timingFunction, init);
    init.animatedValues.push_back(new AnimatedValue(from));
    init.animatedValues.push_back(new AnimatedValue(to));

    auto task =
        new ActiveLengthAnimationTask(init, lengthValueGetter(m_toStyle));
    m_executor->registerTransition(task);
    m_gotTransition = true;
}

void TransitionApplier::applyBackgroundPositionX(double duration, double delay,
                                                 TimingFunction* timingFunction)
{
    if (!canRegisterTransition(
            CSSStyleValuePair::KeyKind::BackgroundPositionX)) {
        return;
    }
    if (m_fromStyle->hasBlockLikeDisplay() == false ||
        m_toStyle->hasBlockLikeDisplay() == false) {
        // TODO Inline Element
        return;
    }
    FrameBox* oldPaintingBox = m_oldFrame->asFrameBox();
    size_t layerSize = m_toStyle->backgroundLayerSize();
    for (size_t i = 0; i < layerSize; i++) {
        AnimatedValue pos1, pos2;
        if (AnimationUtil::backgroundPosXToAnimatedValue(
                m_fromStyle, m_toStyle, oldPaintingBox, m_element, pos1, pos2,
                i)) {
            ActiveAnimationTaskInit init;
            fillActiveAnimationTaskInitForTransition(
                m_element, CSSStyleValuePair::KeyKind::BackgroundPositionX,
                duration, delay, timingFunction, init);
            init.animatedValues.push_back(new AnimatedValue(pos1));
            init.animatedValues.push_back(new AnimatedValue(pos2));
            init.layerIndex = i;

            auto task = new ActiveLengthAnimationTask(
                init, m_toStyle->backgroundPositionX(i));
            m_executor->registerTransition(task);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyBackgroundPositionY(double duration, double delay,
                                                 TimingFunction* timingFunction)
{
    if (!canRegisterTransition(
            CSSStyleValuePair::KeyKind::BackgroundPositionY)) {
        return;
    }
    if (m_fromStyle->hasBlockLikeDisplay() == false ||
        m_toStyle->hasBlockLikeDisplay() == false) {
        // TODO Inline Element
        return;
    }
    FrameBox* oldPaintingBox = m_oldFrame->asFrameBox();
    size_t layerSize = m_toStyle->backgroundLayerSize();
    for (size_t i = 0; i < layerSize; i++) {
        AnimatedValue pos1, pos2;
        if (AnimationUtil::backgroundPosYToAnimatedValue(
                m_fromStyle, m_toStyle, oldPaintingBox, m_element, pos1, pos2,
                i)) {
            ActiveAnimationTaskInit init;
            fillActiveAnimationTaskInitForTransition(
                m_element, CSSStyleValuePair::KeyKind::BackgroundPositionY,
                duration, delay, timingFunction, init);
            init.animatedValues.push_back(new AnimatedValue(pos1));
            init.animatedValues.push_back(new AnimatedValue(pos2));
            init.layerIndex = i;

            auto task = new ActiveLengthAnimationTask(
                init, m_toStyle->backgroundPositionY(i));
            m_executor->registerTransition(task);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyBackgroundSize(double duration, double delay,
                                            TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::KeyKind::BackgroundSize)) {
        return;
    }
    if (m_fromStyle->hasBlockLikeDisplay() == false ||
        m_toStyle->hasBlockLikeDisplay() == false) {
        // TODO Inline Element
        return;
    }
    FrameBox* oldPaintingBox = m_oldFrame->asFrameBox();
    size_t layerSize = m_toStyle->backgroundLayerSize();
    for (size_t i = 0; i < layerSize; i++) {
        AnimatedValue size1, size2;
        if (AnimationUtil::backgroundSizeToAnimatedValue(
                m_fromStyle, m_toStyle, oldPaintingBox, m_element, size1, size2,
                i)) {
            ActiveAnimationTaskInit init;
            fillActiveAnimationTaskInitForTransition(
                m_element, CSSStyleValuePair::KeyKind::BackgroundSize, duration,
                delay, timingFunction, init);
            init.animatedValues.push_back(new AnimatedValue(size1));
            init.animatedValues.push_back(new AnimatedValue(size2));
            init.layerIndex = i;

            auto task = new ActiveLengthSizeAnimationTask(
                init, m_toStyle->backgroundSizeLengthValue(i));
            m_executor->registerTransition(task);
            m_gotTransition = true;
        }
    }
}

void TransitionApplier::applyVisibility(double duration, double delay,
                                        TimingFunction* timingFunction)
{
    if (!canRegisterTransition(CSSStyleValuePair::KeyKind::Visibility)) {
        return;
    }
    ActiveAnimationTaskInit init;
    fillActiveAnimationTaskInitForTransition(
        m_element, CSSStyleValuePair::KeyKind::Visibility, duration, delay,
        timingFunction, init);
    init.animatedValues.push_back(new AnimatedValue(m_fromStyle->visibility()));
    init.animatedValues.push_back(new AnimatedValue(m_toStyle->visibility()));

    auto task = new ActiveVisibilityAnimationTask(init);
    m_executor->registerTransition(task);
    m_gotTransition = true;
}

} // namespace Starfish
