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
#include "core/style/CSSProperty.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

#define STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT()              \
    STARFISH_ASSERT(m_fromValue.isLength() && m_toValue.isLength()); \
    STARFISH_ASSERT((m_fromValue.getLength().isFixed() &&            \
                     m_toValue.getLength().isFixed()) ||             \
                    (m_fromValue.getLength().isPercent() &&          \
                     m_toValue.getLength().isPercent()));

template <typename T>
static float interpolate(const T from, const T to, float progress)
{
    if (from < to) {
        return from + (to - from) * progress;
    }
    return from - (from - to) * progress;
}

static bool shouldCancelPrevious(const AnimationTask* oldTask,
                                 const AnimationTask* newTask)
{
    CSSStyleValuePair::KeyKind type = oldTask->propertyType();
    bool matchBasic = type == newTask->propertyType() &&
                      oldTask->targetElement() == newTask->targetElement();
    if (type == CSSStyleValuePair::BackgroundPositionX ||
        type == CSSStyleValuePair::BackgroundPositionY ||
        type == CSSStyleValuePair::BackgroundSize) {
        return matchBasic && oldTask->extraData() == newTask->extraData();
    }
    return matchBasic;
}

AnimationTask::AnimationTask(Element* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             AnimatedValue from, AnimatedValue to,
                             float durationInms, float delayInms,
                             AnimationTimingFunction* timingFunction,
                             void* data)
{
    m_isStartEventFired = false;
    m_targetElement = target;
    m_durationMs = durationInms;
    m_delayMs = delayInms;
    m_startTimeMs = 0;
    m_fromValue = from;
    m_toValue = to;
    m_property = targetProperty;
    m_timingFunction = timingFunction;
    m_targetPropertyString = CSSPropertyHelper::toGCString(targetProperty);
    m_extraData = data;
}

void AnimationTask::attachedToElement()
{
    m_targetElement->style()->markUsedInAnimator();
}

void AnimationTask::fireStartEvent()
{
    m_isStartEventFired = true;
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

static void checkNeedsUpdateInheritStyleValues(
    CSSStyleValuePair::KeyKind keyKind, Element* parentElement,
    Unit::Color nextColor)
{
    ComputedStyle* parentStyle = parentElement->style();
    Unit::Color parentClr;
    if (keyKind == CSSStyleValuePair::KeyKind::Color) {
        parentClr = parentStyle->color();
    } else if (keyKind == CSSStyleValuePair::KeyKind::CaretColor) {
        parentClr = parentStyle->caretColor();
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    Node* n = parentElement->firstChild();
    while (n) {
        bool isMatch = false;
        ComputedStyle* childStyle = n->style();
        if (childStyle->color() == parentClr &&
            keyKind == CSSStyleValuePair::KeyKind::Color) {
            isMatch = true;
        } else if (childStyle->caretColor() == parentClr &&
                   keyKind == CSSStyleValuePair::KeyKind::CaretColor) {
            isMatch = true;
        }

        if (isMatch) {
            if (n->isElement()) {
                n->setNeedsStyleRecalc(
                    Node::StyleChangeReason::JustNeedsRecalcSelf);
            } else {
                if (keyKind == CSSStyleValuePair::KeyKind::Color) {
                    childStyle->setColor(nextColor);
                } else if (keyKind == CSSStyleValuePair::KeyKind::CaretColor) {
                    childStyle->setCaretColor(nextColor);
                }
            }
        }

        n = n->nextSibling();
    }
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

    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        style->setBackgroundColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottomColor) {
        style->setBorderBottomColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeftColor) {
        style->setBorderLeftColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRightColor) {
        style->setBorderRightColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTopColor) {
        style->setBorderTopColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::Color) {
        checkNeedsUpdateInheritStyleValues(CSSStyleValuePair::KeyKind::Color,
                                           current, Unit::Color(r, g, b, a));
        style->setColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::CaretColor) {
        checkNeedsUpdateInheritStyleValues(
            CSSStyleValuePair::KeyKind::CaretColor, current,
            Unit::Color(r, g, b, a));
        style->setCaretColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::OutlineColor) {
        style->setOutlineColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::TextDecorationColor) {
        style->setTextDecorationColor(Unit::Color(r, g, b, a));
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    current->setNeedsPainting();
}

void LengthAnimationTask::computeToValue()
{
    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        LayoutUnit currentWidth =
            targetElement()->frame()->asFrameBox()->width();
        if (targetElement()->style()->boxSizing() ==
            BoxSizingValue::ContentBoxBoxSizingValue) {
            currentWidth =
                targetElement()->frame()->asFrameBox()->contentWidth();
        }

        m_toFixedValue = currentWidth;
        targetElement()->style()->setWidth(
            Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        LayoutUnit currentHeight =
            targetElement()->frame()->asFrameBox()->height();
        if (targetElement()->style()->boxSizing() ==
            BoxSizingValue::ContentBoxBoxSizingValue) {
            currentHeight =
                targetElement()->frame()->asFrameBox()->contentHeight();
        }

        m_toFixedValue = currentHeight;
        targetElement()->style()->setHeight(
            Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
    } else if (m_property == CSSStyleValuePair::KeyKind::MinWidth) {
        FrameBox* cb = containingBlock(targetElement()->frame());
        m_toFixedValue =
            targetElement()->frame()->style()->minWidth().specifiedValue(
                cb->contentWidth(), targetElement()->frame());
        targetElement()->style()->setMinWidth(
            Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
    } else if (m_property == CSSStyleValuePair::KeyKind::MinHeight) {
        FrameBox* cb = containingBlock(targetElement()->frame());
        m_toFixedValue =
            targetElement()->frame()->style()->minHeight().specifiedValue(
                cb->contentHeight(), targetElement()->frame());
        targetElement()->style()->setMinHeight(
            Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxWidth) {
        FrameBox* cb = containingBlock(targetElement()->frame());
        m_toFixedValue =
            targetElement()->frame()->style()->maxWidth().specifiedValue(
                cb->contentWidth(), targetElement()->frame());
        targetElement()->style()->setMaxWidth(
            Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxHeight) {
        FrameBox* cb = containingBlock(targetElement()->frame());
        m_toFixedValue =
            targetElement()->frame()->style()->maxHeight().specifiedValue(
                cb->contentHeight(), targetElement()->frame());
        targetElement()->style()->setMaxHeight(
            Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    targetElement()->setNeedsLayout();
}

void LengthAnimationTask::attachedToElement()
{
    AnimationTask::attachedToElement();

    if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionX) {
        STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT();
        targetElement()->style()->setBackgroundPositionX(
            m_fromValue.getLength(), (size_t)m_extraData);
        return;
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionY) {
        STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT();
        targetElement()->style()->setBackgroundPositionY(
            m_fromValue.getLength(), (size_t)m_extraData);
        return;
    }

    if (m_toValue.getLength().isDefinite(false)) {
        m_toFixedValue =
            m_toValue.getLength().specifiedValue(0, targetElement()->frame());

        if (m_property == CSSStyleValuePair::KeyKind::Width) {
            targetElement()->style()->setWidth(
                Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
        } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
            targetElement()->style()->setHeight(
                Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
        } else if (m_property == CSSStyleValuePair::KeyKind::MinWidth) {
            targetElement()->style()->setMinWidth(
                Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
        } else if (m_property == CSSStyleValuePair::KeyKind::MinHeight) {
            targetElement()->style()->setMinHeight(
                Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
        } else if (m_property == CSSStyleValuePair::KeyKind::MaxWidth) {
            targetElement()->style()->setMaxWidth(
                Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
        } else if (m_property == CSSStyleValuePair::KeyKind::MaxHeight) {
            targetElement()->style()->setMaxHeight(
                Length(Length::Fixed, (float)m_fromValue.getLayoutUnit()));
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        targetElement()
            ->document()
            ->browsingContext()
            ->webView()
            ->addDidLayoutCallback(
                [](void* data) -> bool {
                    LengthAnimationTask* task = (LengthAnimationTask*)data;
                    task->computeToValue();
                    return true;
                },
                this);
    }
}

Length LengthAnimationTask::interpolateFixed(float progress) const
{
    return Length(Length::Fixed, interpolate(m_fromValue.getLayoutUnit(),
                                             m_toFixedValue, progress));
}

Length LengthAnimationTask::interpolateFixedOrPercent(float progress) const
{
    STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT();
    const Length& from = m_fromValue.getLength();
    const Length& to = m_toValue.getLength();
    return Length(from.type(),
                  interpolate(from.numberData(), to.numberData(), progress));
}

void LengthAnimationTask::execute(float progress)
{
    Element* current = targetElement();
    ComputedStyle* style = current->style();
    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        style->setWidth(interpolateFixed(progress));
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        style->setHeight(interpolateFixed(progress));
    } else if (m_property == CSSStyleValuePair::KeyKind::MinWidth) {
        style->setMinWidth(interpolateFixed(progress));
    } else if (m_property == CSSStyleValuePair::KeyKind::MinHeight) {
        style->setMinHeight(interpolateFixed(progress));
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxWidth) {
        style->setMaxWidth(interpolateFixed(progress));
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxHeight) {
        style->setMaxHeight(interpolateFixed(progress));
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionX) {
        style->setBackgroundPositionX(interpolateFixedOrPercent(progress),
                                      (size_t)m_extraData);
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionY) {
        style->setBackgroundPositionY(interpolateFixedOrPercent(progress),
                                      (size_t)m_extraData);
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    current->setNeedsLayout();
}

void LengthSizeAnimationTask::attachedToElement()
{
    STARFISH_ASSERT(m_fromValue.isLengthSize() && m_toValue.isLengthSize());
    STARFISH_ASSERT(m_fromValue.getLengthSize().width().type() ==
                    m_toValue.getLengthSize().width().type());
    STARFISH_ASSERT(m_fromValue.getLengthSize().height().type() ==
                    m_toValue.getLengthSize().height().type());

    AnimationTask::attachedToElement();
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        targetElement()->style()->setBackgroundSize(m_fromValue.getLengthSize(),
                                                    (size_t)m_extraData);
        return;
    }
}

LengthSize LengthSizeAnimationTask::interpolateFixedOrPercent(
    float progress) const
{
#define INTERPOLATE_LENGTHSIZE(WH)                               \
    Length(from.WH().type(), interpolate(from.WH().numberData(), \
                                         to.WH().numberData(), progress))

    const LengthSize& from = m_fromValue.getLengthSize();
    const LengthSize& to = m_toValue.getLengthSize();
    STARFISH_ASSERT(from.width().type() != Length::Auto ||
                    from.height().type() != Length::Auto);
    STARFISH_ASSERT(to.width().type() != Length::Auto ||
                    to.height().type() != Length::Auto);

    if (from.width().type() == Length::Auto) {
        return LengthSize(Length(), INTERPOLATE_LENGTHSIZE(height));
    } else if (from.height().type() == Length::Auto) {
        return LengthSize(INTERPOLATE_LENGTHSIZE(width), Length());
    }
    return LengthSize(INTERPOLATE_LENGTHSIZE(width),
                      INTERPOLATE_LENGTHSIZE(height));
#undef INTERPOLATE_LENGTHSIZE
}

void LengthSizeAnimationTask::execute(float progress)
{
    Element* current = targetElement();
    ComputedStyle* style = current->style();
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        style->setBackgroundSize(interpolateFixedOrPercent(progress),
                                 (size_t)m_extraData);
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    current->setNeedsLayout();
}

OpacityAnimationTask::OpacityAnimationTask(
    Element* target, AnimatedValue fromValue, AnimatedValue toValue,
    float duration, float delay, AnimationTimingFunction* timingFunction,
    void* data)
    : AnimationTask(target, CSSStyleValuePair::KeyKind::Opacity, fromValue,
                    toValue, duration, delay, timingFunction, data)
{
}

void OpacityAnimationTask::opacityUpdated(bool before, bool after)
{
    Element* current = targetElement();
    Frame* frame = current->frame();

    if (before != after) {
        targetElement()->webView()->clearStackingContext(true);
    }

    if (frame->isFrameBox() && frame->asFrameBox()->stackingContext() &&
        frame->asFrameBox()->needsGraphicsBuffer()) {
        targetElement()->setNeedsComposite();
    } else {
        targetElement()->setNeedsPainting();
    }
}

void OpacityAnimationTask::detachedFromElement()
{
    AnimationTask::detachedFromElement();

    Element* current = targetElement();
    Frame* frame = current->frame();

    bool before = frame->isEstablishesStackingContext();
    ComputedStyle* style = current->style();
    style->setOpacity(m_toValue.getFloat());
    frame->computeStyleFlags();
    bool after = frame->isEstablishesStackingContext();
    opacityUpdated(before, after);
}

void OpacityAnimationTask::execute(float progress)
{
    float from = m_fromValue.getFloat();
    float to = m_toValue.getFloat();

    Element* current = targetElement();
    Frame* frame = current->frame();
    bool before = frame->isEstablishesStackingContext();
    ComputedStyle* style = current->style();
    float newOpacity = from * (1 - progress) + to * progress;
    style->setOpacity(newOpacity);
    frame->computeStyleFlags();
    bool after = frame->isEstablishesStackingContext();
    opacityUpdated(before, after);
}

inline double rad2deg(double rad)
{
    return rad * (180.0 / M_PI);
}

inline double deg2rad(float degree)
{
    return degree * M_PI / 180;
}

/// https://drafts.csswg.org/css-transforms/#decomposing-a-2d-matrix
static MatrixDecomposed2D decomposing2DMatrix(const SkMatrix& matrix)
{
    // 0(row0x) 1(row1x) 2
    // 3(row0y) 4(row1y) 5
    // 6 7 8
    float row0x = matrix.get(0); // m11
    float row0y = matrix.get(3); // m12
    float row1x = matrix.get(1); // m21
    float row1y = matrix.get(4); // m22

    float translateX = matrix.getTranslateX();
    float translateY = matrix.getTranslateY();
    float scaleX = sqrt(row0x * row0x + row0y * row0y);
    float scaleY = sqrt(row1x * row1x + row1y * row1y);

    // If determinant is negative, one axis was flipped.
    float determinant = row0x * row1y - row0y * row1x;
    if (determinant < 0) {
        if (row0x < row1y) {
            // Flip axis with minimum unit vector dot product.
            scaleX = -scaleX;
        } else {
            scaleY = -scaleY;
        }
    }

    // Renormalize matrix to remove scale.
    if (scaleX != 0.0) {
        row0x *= 1. / scaleX;
        row0y *= 1. / scaleX;
    }

    if (scaleY != 0.0) {
        row1x *= 1. / scaleY;
        row1y *= 1. / scaleY;
    }

    // Compute rotation and renormalize matrix.
    float angle = atan2(row0y, row0x);
    if (angle != 0.0) {
        // Rotate(-angle) = [cos(angle), sin(angle), -sin(angle), cos(angle)]
        //                = [row0x, -row0y, row0y, row0x]
        // Thanks to the normalization above.
        float sn = -row0y;
        float cs = row0x;
        float m11 = row0x;
        float m12 = row0y;
        float m21 = row1x;
        float m22 = row1y;
        row0x = cs * m11 + sn * m21;
        row0y = cs * m12 + sn * m22;
        row1x = -sn * m11 + cs * m21;
        row1y = -sn * m12 + cs * m22;
    }

    MatrixDecomposed2D ret;
    ret.matrixM11 = row0x;
    ret.matrixM12 = row0y;
    ret.matrixM21 = row1x;
    ret.matrixM22 = row1y;
    // Convert into degrees because our rotation functions expect it.
    ret.angle = rad2deg(angle);
    ret.scaleX = scaleX;
    ret.scaleY = scaleY;
    ret.translateX = translateX;
    ret.translateY = translateY;

    return ret;
}

static void matrixInterpolationPreprocessing(MatrixDecomposed2D& a,
                                             MatrixDecomposed2D& b)
{
    // If x-axis of one is flipped, and y-axis of the other,
    // convert to an unflipped rotation.
    if ((a.scaleX < 0 && b.scaleY < 0) || (a.scaleY < 0 && b.scaleX < 0)) {
        a.scaleX = -a.scaleX;
        a.scaleY = -a.scaleY;
        a.angle += a.angle < 0 ? 180 : -180;
    }
    // Don’t rotate the long way around.
    // XXX: we don't need this step I think
    /*
    if (!a.angle) {
        a.angle = 360;
    }
    if (!b.angle) {
        b.angle = 360;
    }*/

    if (std::abs(a.angle - b.angle) > 180) {
        if (a.angle > b.angle) {
            a.angle -= 360;
        } else {
            b.angle -= 360;
        }
    }
}

// https://drafts.csswg.org/css-transforms/#recomposing-to-a-2d-matrix
static SkMatrix recomposing2DMatrix(const MatrixDecomposed2D& decomposed)
{
    SkMatrix matrix = SkMatrix::I();

    // 0(M11) 1(M21) 2
    // 3(M12) 4(M22) 5
    // 6 7 8
    matrix.set(0, decomposed.matrixM11);
    matrix.set(3, decomposed.matrixM12);
    matrix.set(1, decomposed.matrixM21);
    matrix.set(4, decomposed.matrixM22);

    // Translate matrix.
    matrix.set(2, decomposed.translateX * decomposed.matrixM11 +
                      decomposed.translateY * decomposed.matrixM21);
    matrix.set(5, decomposed.translateX * decomposed.matrixM12 +
                      decomposed.translateY * decomposed.matrixM22);

    // Rotate matrix.
    float angle = deg2rad(decomposed.angle);
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    SkMatrix rotateMatrix = SkMatrix::I();

    rotateMatrix.set(0, cosAngle);
    rotateMatrix.set(3, sinAngle);
    rotateMatrix.set(1, -sinAngle);
    rotateMatrix.set(4, cosAngle);

    // Multiplication of matrix and rotate_matrix
    SkMatrix newMatrix = SkMatrix::I();
    newMatrix.setConcat(matrix, rotateMatrix);

    // Scale matrix.
    newMatrix.set(0, newMatrix[0] * decomposed.scaleX);
    newMatrix.set(3, newMatrix[3] * decomposed.scaleX);
    newMatrix.set(1, newMatrix[1] * decomposed.scaleY);
    newMatrix.set(4, newMatrix[4] * decomposed.scaleY);

    return newMatrix;
}

TransformAnimationTask::TransformAnimationTask(
    Element* target, AnimatedValue fromValue, float duration, float delay,
    AnimationTimingFunction* timingFunction)
    : AnimationTask(target, CSSStyleValuePair::KeyKind::Transform, fromValue,
                    AnimatedValue(), duration, delay, timingFunction)
    , m_decomposedFrom(decomposing2DMatrix(fromValue.getMatrix()))
{
    target->style()->rareComputedStyleData()->ensureTransforms();
}

void TransformAnimationTask::computeToValue()
{
    FrameBox* box = targetElement()->frame()->asFrameBox();
    SkMatrix matrix = box->style()->transformsToMatrix(
        box->width(), box->height(), box, true);
    m_toValue = AnimatedValue(matrix);
    m_decomposedTo = decomposing2DMatrix(m_toValue.getMatrix());

    matrixInterpolationPreprocessing(m_decomposedFrom, m_decomposedTo);

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
    targetElement()
        ->document()
        ->browsingContext()
        ->webView()
        ->addDidLayoutCallback(
            [](void* data) {
                TransformAnimationTask* task = (TransformAnimationTask*)data;
                task->computeToValue();
                return false;
            },
            this);
}

void TransformAnimationTask::detachedFromElement()
{
    AnimationTask::detachedFromElement();

    Element* current = targetElement();
    ComputedStyle* style = current->style();

    if (style && style->hasTransforms() && current->frame()) {
        FrameBox* box = current->frame()->asFrameBox();
        auto transforms = style->rareComputedStyleData()->transforms();
        if (transforms->at(transforms->size() - 1).type() ==
            StyleTransformData::InternalMatrix) {
            // cleanup
            transforms->removeAt(transforms->size() - 1);
            if (transforms->size() == 0) {
                current->webView()->clearStackingContext(true);
                style->clearTransform();
                box->computeStyleFlags();
                current->setNeedsPainting();
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

    MatrixDecomposed2D now;

    now.angle = m_decomposedFrom.angle * (1 - progress) +
                m_decomposedTo.angle * progress;
    now.matrixM11 = m_decomposedFrom.matrixM11 * (1 - progress) +
                    m_decomposedTo.matrixM11 * progress;
    now.matrixM12 = m_decomposedFrom.matrixM12 * (1 - progress) +
                    m_decomposedTo.matrixM12 * progress;
    now.matrixM21 = m_decomposedFrom.matrixM21 * (1 - progress) +
                    m_decomposedTo.matrixM21 * progress;
    now.matrixM22 = m_decomposedFrom.matrixM22 * (1 - progress) +
                    m_decomposedTo.matrixM22 * progress;
    now.scaleX = m_decomposedFrom.scaleX * (1 - progress) +
                 m_decomposedTo.scaleX * progress;
    now.scaleY = m_decomposedFrom.scaleY * (1 - progress) +
                 m_decomposedTo.scaleY * progress;
    now.translateX = m_decomposedFrom.translateX * (1 - progress) +
                     m_decomposedTo.translateX * progress;
    now.translateY = m_decomposedFrom.translateY * (1 - progress) +
                     m_decomposedTo.translateY * progress;

    data.setInternalMatrix(recomposing2DMatrix(now));
    current->webView()->setNeedsComputeStackingContextProperties();
}

// [NOTICE]
// registerAnimation will be replaced 'createAnimation'
// Creation of AnimationTask will happen in Animation Executor.
void AnimationExecutor::registerAnimation(AnimationTask* newTask)
{
    startIfNeeds();
    cancelPreviousAnimationIfNeeded(newTask);
    newTask->attachedToElement();
    m_animationList.push_back(newTask);
}

// This function cancel previous animation
// which is related taget node and its property.
// * This function is called when we need new animation.
void AnimationExecutor::cancelPreviousAnimationIfNeeded(AnimationTask* newTask)
{
    // fire end event
    m_animationList.erase(
        std::remove_if(m_animationList.begin(), m_animationList.end(),
                       [&newTask](AnimationTask* current) {
                           if (shouldCancelPrevious(current, newTask)) {
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
    window()->webView()->increaseActiveAnimatorCount();
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
        window()->webView()->decreaseActiveAnimatorCount();
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
            task->m_startTimeMs = currentTickCount + task->m_delayMs;
        }

        if (task->m_startTimeMs <= currentTickCount) {
            if (!task->m_isStartEventFired) {
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

        if (currentElementStyle && currentElementStyle->transition()) {
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

#undef STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT
