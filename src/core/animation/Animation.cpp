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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/animation/Animation.h"
#include "core/animation/AnimationUtil.h"
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

namespace Starfish {

template <typename T>
static float interpolate(const T from, const T to, float progress)
{
    if (from < to) {
        return from + (to - from) * progress;
    }
    return from - (from - to) * progress;
}

void AnimationExecutor::checkActiveAnimationExecutorInWebView()
{
    auto& v = window()->webView()->m_activeAnimationExecutor;

    if (m_activeAnimations.size()) {
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i] == this) {
                return;
            }
        }
        v.push_back(this);
    } else {
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i] == this) {
                v.erase(i);
                return;
            }
        }
    }
}

ActiveAnimationTask::ActiveAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const AnimatedValue& from, const AnimatedValue& to, uint64_t durationInms,
    uint64_t delayInms, AnimationTimingFunction* timingFunction)
    : m_property(targetProperty)
    , m_targetElement(target)
    , m_fromValue(from)
    , m_toValue(to)
    , m_startTimeMs(0)
    , m_durationMs(durationInms)
    , m_delayMs(delayInms)
    , m_timingFunction(timingFunction)
{
}

void ActiveAnimationTask::step(uint64_t currentTickCount, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    float f = 0;
    if (m_startTimeMs != 0) {
        f = fraction(currentTickCount);
    }

    execute(computeProgress(f), style);
}

void ActiveAnimationTask::fireStartEvent()
{
    // STARFISH_LOG_INFO("element %p property %s transitionStart\n",
    // m_targetElement, CSSPropertyHelper::toString(m_property));
    TransitionEventInit init;
    init.setPropertyName(CSSPropertyHelper::toGCString(m_property));
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    TransitionEvent* event =
        new TransitionEvent(m_targetElement->executionContext(),
                            m_targetElement->starfish()
                                ->staticStrings()
                                ->m_transitionstart.localName(),
                            init);
    m_targetElement->dispatchEventIdleTimeByUA(event);
}

void ActiveAnimationTask::fireEndEvent()
{
    // STARFISH_LOG_INFO("element %p property %s transitionEnd\n",
    // m_targetElement, CSSPropertyHelper::toString(m_property));
    TransitionEventInit init;
    init.setPropertyName(CSSPropertyHelper::toGCString(m_property));
    init.setBubbles(true);
    init.setCancelable(true);
    // TODO add more information to init
    TransitionEvent* event = new TransitionEvent(
        m_targetElement->executionContext(), m_targetElement->starfish()
                                                 ->staticStrings()
                                                 ->m_transitionend.localName(),
        init);
    m_targetElement->dispatchEventIdleTimeByUA(event);
}

void ActiveAnimationTask::fireCancelEvent()
{
    // STARFISH_LOG_INFO("element %p property %s transitionCancel\n",
    // m_targetElement, CSSPropertyHelper::toString(m_property));
    TransitionEventInit init;
    init.setPropertyName(CSSPropertyHelper::toGCString(m_property));
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    TransitionEvent* event =
        new TransitionEvent(m_targetElement->executionContext(),
                            m_targetElement->starfish()
                                ->staticStrings()
                                ->m_transitioncancel.localName(),
                            init);
    m_targetElement->dispatchEventIdleTimeByUA(event);
}

void ActiveOpacityAnimationTask::execute(float progress, ComputedStyle* style)
{
    float from = m_fromValue.getFloat();
    float to = m_toValue.getFloat();
    float newOpacity = from * (1 - progress) + to * progress;
    style->setOpacity(newOpacity);
}

bool ActiveOpacityAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    if (newStyle->opacity() != m_toValue.getFloat()) {
        return false;
    }
    return true;
}

void ActiveOpacityAnimationTask::attachToElement(ComputedStyle* style)
{
    m_targetElement->markRunningOpacityAnimation();
}

void ActiveOpacityAnimationTask::detachFromElement(ComputedStyle* style)
{
    m_targetElement->clearRunningOpacityAnimation();
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
static ActiveTransformAnimationTask::MatrixDecomposed2D decomposing2DMatrix(
    const SkMatrix& matrix)
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

    ActiveTransformAnimationTask::MatrixDecomposed2D ret;
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

static void matrixInterpolationPreprocessing(
    ActiveTransformAnimationTask::MatrixDecomposed2D& a,
    ActiveTransformAnimationTask::MatrixDecomposed2D& b)
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
static SkMatrix recomposing2DMatrix(
    const ActiveTransformAnimationTask::MatrixDecomposed2D& decomposed)
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

ActiveTransformAnimationTask::ActiveTransformAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const AnimatedValue& from, const AnimatedValue& to, uint64_t durationInms,
    uint64_t delayInms, AnimationTimingFunction* timingFunction,
    StyleTransformDataGroup* orgTransformValue)
    : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                          delayInms, timingFunction)
    , m_originalTransformValue(nullptr)
    , m_decomposedFrom(decomposing2DMatrix(m_fromValue.getMatrix()))
    , m_decomposedTo(decomposing2DMatrix(m_toValue.getMatrix()))
{
    if (orgTransformValue) {
        StyleTransformDataGroup* newOrgData = new StyleTransformDataGroup();
        for (size_t i = 0; i < orgTransformValue->size(); i++) {
            newOrgData->append(orgTransformValue->at(i));
        }
        m_originalTransformValue = newOrgData;
    }
    matrixInterpolationPreprocessing(m_decomposedFrom, m_decomposedTo);
}

void ActiveTransformAnimationTask::execute(float progress, ComputedStyle* style)
{
    SkMatrix from = m_fromValue.getMatrix();
    SkMatrix to = m_toValue.getMatrix();
    Element* current = targetElement();
    auto transforms = style->rareComputedStyleData()->transforms();

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

    auto transform = new StyleTransformDataGroup();
    SkMatrix newMatrix = recomposing2DMatrix(now);

    StyleTransformData m(StyleTransformData::OperationType::Matrix);
    m.setMatrix(newMatrix[0], newMatrix[3], newMatrix[1], newMatrix[4],
                newMatrix[2], newMatrix[5]);

    transform->append(m);
    style->setTransform(transform);
}

bool ActiveTransformAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    if (newStyle->transforms() == nullptr) {
        if (m_originalTransformValue == nullptr) {
            return true;
        }
        return false;
    }
    if (m_originalTransformValue == nullptr) {
        if (newStyle->transforms() == nullptr) {
            return true;
        }
        return false;
    }
    if (*newStyle->transforms() != *m_originalTransformValue) {
        return false;
    }
    return true;
}

void ActiveTransformAnimationTask::attachToElement(ComputedStyle* style)
{
    m_targetElement->markRunningTransformAnimation();
}

void ActiveTransformAnimationTask::detachFromElement(ComputedStyle* style)
{
    m_targetElement->clearRunningTransformAnimation();
}

void ActiveColorAnimationTask::execute(float progress, ComputedStyle* style)
{
    Unit::Color from = m_fromValue.getColor();
    Unit::Color to = m_toValue.getColor();

    unsigned char r = from.r() * (1 - progress) + to.r() * progress;
    unsigned char g = from.g() * (1 - progress) + to.g() * progress;
    unsigned char b = from.b() * (1 - progress) + to.b() * progress;
    unsigned char a = from.a() * (1 - progress) + to.a() * progress;

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
        style->setColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::CaretColor) {
        style->setCaretColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::OutlineColor) {
        style->setOutlineColor(Unit::Color(r, g, b, a));
    } else if (m_property == CSSStyleValuePair::KeyKind::TextDecorationColor) {
        style->setTextDecorationColor(Unit::Color(r, g, b, a));
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

bool ActiveColorAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        if (newStyle->backgroundColor() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottomColor) {
        if (newStyle->border().bottom().color() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeftColor) {
        if (newStyle->border().left().color() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRightColor) {
        if (newStyle->border().right().color() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTopColor) {
        if (newStyle->border().top().color() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Color) {
        if (newStyle->color() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::CaretColor) {
        if (newStyle->caretColor() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::OutlineColor) {
        if (newStyle->outlineColor() == m_toValue.getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::TextDecorationColor) {
        if (newStyle->textDecorationColor() == m_toValue.getColor()) {
            return true;
        }
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    return false;
}

ActiveLengthAnimationTask::ActiveLengthAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const AnimatedValue& from, const AnimatedValue& to, uint64_t durationInms,
    uint64_t delayInms, AnimationTimingFunction* timingFunction,
    Length originalToValue, size_t indexForBgLayer)
    : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                          delayInms, timingFunction)
    , m_originalToValue(originalToValue)
    , m_indexForBgLayer(indexForBgLayer)
{
}

void ActiveLengthAnimationTask::execute(float progress, ComputedStyle* style)
{
    Length newLength;
    if (m_toValue.getLength().isPercent()) {
        float fromPercent = m_fromValue.getLength().percent();
        float toPercent = m_toValue.getLength().percent();
        newLength = Length(Length::Percent,
                           interpolate(fromPercent, toPercent, progress));
    } else {
        float fromFixed = m_fromValue.getLength().fixed();
        float toFixed = m_toValue.getLength().fixed();
        newLength =
            Length(Length::Fixed, interpolate(fromFixed, toFixed, progress));
    }

    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        style->setWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        style->setHeight(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginTop) {
        style->setMarginTop(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginRight) {
        style->setMarginRight(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginBottom) {
        style->setMarginBottom(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginLeft) {
        style->setMarginLeft(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MinWidth) {
        style->setMinWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MinHeight) {
        style->setMinHeight(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxWidth) {
        style->setMaxWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxHeight) {
        style->setMaxHeight(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingTop) {
        style->setPaddingTop(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingRight) {
        style->setPaddingRight(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingBottom) {
        style->setPaddingBottom(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingLeft) {
        style->setPaddingLeft(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTop) {
        style->setBorderTopWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRight) {
        style->setBorderRightWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottom) {
        style->setBorderBottomWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeft) {
        style->setBorderLeftWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::Left) {
        style->setLeft(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::Top) {
        style->setTop(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::Right) {
        style->setRight(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::Bottom) {
        style->setBottom(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionX) {
        style->setBackgroundPositionX(newLength, m_indexForBgLayer);
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionY) {
        style->setBackgroundPositionY(newLength, m_indexForBgLayer);
    } else if (m_property == CSSStyleValuePair::KeyKind::FontSize) {
        style->setFontSize(newLength);
        style->loadFont(m_targetElement);
    }
}

bool ActiveLengthAnimationTask::isKindOfTransitionProperty(
    CSSStyleValuePair::KeyKind k)
{
    if (m_property == CSSStyleValuePair::KeyKind::MarginTop) {
        if (k == CSSStyleValuePair::KeyKind::MarginTop ||
            k == CSSStyleValuePair::KeyKind::Margin) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginRight) {
        if (k == CSSStyleValuePair::KeyKind::MarginRight ||
            k == CSSStyleValuePair::KeyKind::Margin) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginBottom) {
        if (k == CSSStyleValuePair::KeyKind::MarginBottom ||
            k == CSSStyleValuePair::KeyKind::Margin) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginLeft) {
        if (k == CSSStyleValuePair::KeyKind::MarginLeft ||
            k == CSSStyleValuePair::KeyKind::Margin) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTop) {
        if (k == CSSStyleValuePair::KeyKind::BorderTop ||
            k == CSSStyleValuePair::KeyKind::Border) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRight) {
        if (k == CSSStyleValuePair::KeyKind::BorderRight ||
            k == CSSStyleValuePair::KeyKind::Border) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottom) {
        if (k == CSSStyleValuePair::KeyKind::BorderBottom ||
            k == CSSStyleValuePair::KeyKind::Border) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeft) {
        if (k == CSSStyleValuePair::KeyKind::BorderLeft ||
            k == CSSStyleValuePair::KeyKind::Border) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingTop) {
        if (k == CSSStyleValuePair::KeyKind::PaddingTop ||
            k == CSSStyleValuePair::KeyKind::Padding) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingRight) {
        if (k == CSSStyleValuePair::KeyKind::PaddingRight ||
            k == CSSStyleValuePair::KeyKind::Padding) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingBottom) {
        if (k == CSSStyleValuePair::KeyKind::PaddingBottom ||
            k == CSSStyleValuePair::KeyKind::Padding) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingLeft) {
        if (k == CSSStyleValuePair::KeyKind::PaddingLeft ||
            k == CSSStyleValuePair::KeyKind::Padding) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionX) {
        if (k == CSSStyleValuePair::KeyKind::BackgroundPositionX ||
            k == CSSStyleValuePair::KeyKind::BackgroundPosition ||
            k == CSSStyleValuePair::KeyKind::Background) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionY) {
        if (k == CSSStyleValuePair::KeyKind::BackgroundPositionY ||
            k == CSSStyleValuePair::KeyKind::BackgroundPosition ||
            k == CSSStyleValuePair::KeyKind::Background) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::FontSize) {
        if (k == CSSStyleValuePair::KeyKind::FontSize ||
            k == CSSStyleValuePair::KeyKind::Font) {
            return true;
        }
    } else {
        return m_property == k;
    }
    return false;
}

bool ActiveLengthAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        if (m_originalToValue == newStyle->width()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        if (m_originalToValue == newStyle->height()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginTop) {
        if (m_originalToValue == newStyle->margin().top()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginRight) {
        if (m_originalToValue == newStyle->margin().right()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginBottom) {
        if (m_originalToValue == newStyle->margin().bottom()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MarginLeft) {
        if (m_originalToValue == newStyle->margin().left()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MinWidth) {
        if (m_originalToValue == newStyle->minWidth()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MinHeight) {
        if (m_originalToValue == newStyle->minHeight()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxWidth) {
        if (m_originalToValue == newStyle->maxWidth()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::MaxHeight) {
        if (m_originalToValue == newStyle->maxHeight()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTop) {
        if (m_originalToValue == newStyle->border().top().width()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRight) {
        if (m_originalToValue == newStyle->border().right().width()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottom) {
        if (m_originalToValue == newStyle->border().bottom().width()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeft) {
        if (m_originalToValue == newStyle->border().left().width()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingTop) {
        if (m_originalToValue == newStyle->padding().top()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingRight) {
        if (m_originalToValue == newStyle->padding().right()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingBottom) {
        if (m_originalToValue == newStyle->padding().bottom()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::PaddingLeft) {
        if (m_originalToValue == newStyle->padding().left()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Left) {
        if (m_originalToValue == newStyle->left()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Top) {
        if (m_originalToValue == newStyle->top()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Right) {
        if (m_originalToValue == newStyle->right()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Bottom) {
        if (m_originalToValue == newStyle->bottom()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionX) {
        if (m_indexForBgLayer < newStyle->backgroundLayerSize() &&
            m_originalToValue ==
                newStyle->backgroundPositionX(m_indexForBgLayer)) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BackgroundPositionY) {
        if (m_indexForBgLayer < newStyle->backgroundLayerSize() &&
            m_originalToValue ==
                newStyle->backgroundPositionY(m_indexForBgLayer)) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::FontSize) {
        if (m_originalToValue.fixed() == newStyle->fixedFontSize()) {
            return true;
        }
    }
    return false;
}

ActiveLengthSizeAnimationTask::ActiveLengthSizeAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const AnimatedValue& from, const AnimatedValue& to, uint64_t durationInms,
    uint64_t delayInms, AnimationTimingFunction* timingFunction,
    LengthSize originalToValue, size_t indexForBgLayer)
    : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                          delayInms, timingFunction)
    , m_originalToValue(originalToValue)
    , m_indexForBgLayer(indexForBgLayer)
{
}

static LengthSize interpolateLengthSize(float progress, AnimatedValue fromValue,
                                        AnimatedValue toValue)
{
#define INTERPOLATE_LENGTHSIZE(WH)                                 \
    Length(from->WH().type(), interpolate(from->WH().numberData(), \
                                          to->WH().numberData(), progress))

    LengthSize* from = fromValue.getLengthSize();
    LengthSize* to = toValue.getLengthSize();
    STARFISH_ASSERT(from->width().type() == to->width().type());
    STARFISH_ASSERT(from->height().type() == to->height().type());
    STARFISH_ASSERT(from->width().type() != Length::Auto ||
                    from->height().type() != Length::Auto);
    // NOTE
    // (O) FROM(!auto, !auto) -> TO(!auto, !auto)
    // (O) FROM(auto, !auto) -> TO(auto, !auto)
    // (O) FROM(!auto, auto) -> TO(!auto, auto)
    // (X) FROM(auto, auto) -> TO(auto, auto)
    // (X) FROM(auto, !auto) -> TO(!auto, auto)
    // (X) FROM(!auto, auto) -> TO(auto, !auto)
    if (from->width().type() == Length::Auto) {
        return LengthSize(Length(), INTERPOLATE_LENGTHSIZE(height));
    } else if (from->height().type() == Length::Auto) {
        return LengthSize(INTERPOLATE_LENGTHSIZE(width), Length());
    }
    return LengthSize(INTERPOLATE_LENGTHSIZE(width),
                      INTERPOLATE_LENGTHSIZE(height));
#undef INTERPOLATE_LENGTHSIZE
}

void ActiveLengthSizeAnimationTask::execute(float progress,
                                            ComputedStyle* style)
{
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        style->setBackgroundSize(
            interpolateLengthSize(progress, m_fromValue, m_toValue),
            m_indexForBgLayer);
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

bool ActiveLengthSizeAnimationTask::isKindOfTransitionProperty(
    CSSStyleValuePair::KeyKind k)
{
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        if (k == CSSStyleValuePair::KeyKind::BackgroundSize ||
            k == CSSStyleValuePair::KeyKind::Background) {
            return true;
        }
    } else {
        return m_property == k;
    }
    return false;
}

bool ActiveLengthSizeAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        if (m_indexForBgLayer < newStyle->backgroundLayerSize() &&
            newStyle->backgroundSizeIsLength(m_indexForBgLayer) &&
            m_originalToValue ==
                newStyle->backgroundSizeLengthValue(m_indexForBgLayer)) {
            return true;
        }
    }
    return false;
}

static inline bool _checkCSSProperty(CSSStyleValuePair::KeyKind kind,
                                     CSSStyleValuePair::KeyKind a)
{
    return kind == a;
}

static inline bool _checkCSSProperty(CSSStyleValuePair::KeyKind kind,
                                     CSSStyleValuePair::KeyKind a,
                                     CSSStyleValuePair::KeyKind b)
{
    return kind == a || kind == b;
}

static inline bool _checkCSSProperty(CSSStyleValuePair::KeyKind kind,
                                     CSSStyleValuePair::KeyKind a,
                                     CSSStyleValuePair::KeyKind b,
                                     CSSStyleValuePair::KeyKind c)
{
    return kind == a || kind == b || kind == c;
}

#define STARFISH_ASSERT_INPUT_LENGTH_FIXED()                         \
    STARFISH_ASSERT(m_fromValue.isLength() && m_toValue.isLength()); \
    STARFISH_ASSERT(m_fromValue.getLength().isFixed() &&             \
                    m_toValue.getLength().isFixed());

#define STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT()              \
    STARFISH_ASSERT(m_fromValue.isLength() && m_toValue.isLength()); \
    STARFISH_ASSERT((m_fromValue.getLength().isFixed() &&            \
                     m_toValue.getLength().isFixed()) ||             \
                    (m_fromValue.getLength().isPercent() &&          \
                     m_toValue.getLength().isPercent()));

#define _DAMAGED_KEYS(PropName, ...) (damagedKeys[PropName])
#define NEED_TRANSITION(...)       \
    (_DAMAGED_KEYS(__VA_ARGS__) && \
     (isPropertyAll || _checkCSSProperty(property, __VA_ARGS__)))

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* oldStyle, Frame* oldFrame,
    ComputedStyle* newStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, float>>&
        canceledAnimationProgress)
{
    bool ret = false;
    StyleTransitionData* data = newStyle->transition();
    AnimationExecutor* executor = element->document()->animationExecutor();

    for (size_t i = 0; i < data->size(); i++) {
        if (!data->duration(i).toTimeValue()) {
            continue;
        }

        bool gotTransition = false;
        CSSStyleValuePair::KeyKind property = data->property(i);
        bool isPropertyAll = property == CSSStyleValuePair::All;

        auto duration = data->duration(i).toTimeValue();
        auto delay = data->delay(i).toTimeValue();
        auto timingFunction = data->timingFunction(i);

        if (NEED_TRANSITION(CSSStyleValuePair::Opacity)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::Opacity);
            if (!found) {
                auto task = new ActiveOpacityAnimationTask(
                    element, CSSStyleValuePair::Opacity,
                    AnimatedValue(oldStyle->opacity()),
                    AnimatedValue(newStyle->opacity()), duration, delay,
                    timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::Transform)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::Transform);
            if (!found && oldFrame->isTransformable()) {
                FrameBox* box = oldFrame->asFrameBox();
                SkMatrix matrixFrom = oldStyle->transformsToMatrix(
                    box->width(), box->height(), box, true);

                box = oldFrame->asFrameBox();
                SkMatrix matrixTo = newStyle->transformsToMatrix(
                    box->width(), box->height(), box, true);

                auto task = new ActiveTransformAnimationTask(
                    element, CSSStyleValuePair::Transform,
                    AnimatedValue(matrixFrom), AnimatedValue(matrixTo),
                    duration, delay, timingFunction, newStyle->transforms());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        // color series
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundColor,
                            CSSStyleValuePair::Background)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BackgroundColor);
            if (!found) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BackgroundColor,
                    AnimatedValue(oldStyle->backgroundColor()),
                    AnimatedValue(newStyle->backgroundColor()), duration, delay,
                    timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderBottomColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderBottom)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderBottomColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->border().bottom().color();
                Unit::Color newColor = newStyle->border().bottom().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderBottomColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderLeftColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderLeft)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderLeftColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->border().left().color();
                Unit::Color newColor = newStyle->border().left().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderLeftColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderRightColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderRight)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderRightColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->border().right().color();
                Unit::Color newColor = newStyle->border().right().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderRightColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderTopColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderTop)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderTopColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->border().top().color();
                Unit::Color newColor = newStyle->border().top().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderTopColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Color)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::Color);
            if (!found) {
                Unit::Color oldColor = oldStyle->color();
                Unit::Color newColor = newStyle->color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::Color, AnimatedValue(oldColor),
                    AnimatedValue(newColor), duration, delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::CaretColor)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::CaretColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->caretColor();
                Unit::Color newColor = newStyle->caretColor();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::CaretColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::OutlineColor)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::OutlineColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->outlineColor();
                Unit::Color newColor = newStyle->outlineColor();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::OutlineColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::TextDecorationColor,
                            CSSStyleValuePair::TextDecoration)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::TextDecorationColor);
            if (!found) {
                Unit::Color oldColor = oldStyle->textDecorationColor();
                Unit::Color newColor = newStyle->textDecorationColor();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::TextDecorationColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }
        // <- color series

        // length series
        if (NEED_TRANSITION(CSSStyleValuePair::Width) && oldFrame &&
            oldFrame->isFrameBox()) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::Width);
            if (!found) {
                auto oldWidth = oldStyle->width();
                auto newWidth = newStyle->width();

                if ((oldWidth.isPercent() || oldWidth.isFixed()) &&
                    (newWidth.isPercent() || newWidth.isFixed())) {
                    auto fromValue = oldStyle->width();
                    auto toValue = newStyle->width();

                    if (toValue.isPercent() && !fromValue.isPercent()) {
                        FrameBox* cb = containingBlock(oldFrame);
                        if (cb->contentWidth() == 0) {
                            fromValue = Length(Length::Percent, 0);
                        } else if (oldStyle->boxSizing() ==
                                   BoxSizingValue::BorderBoxBoxSizingValue) {
                            fromValue = Length(Length::Percent,
                                               oldFrame->asFrameBox()->width() /
                                                   cb->contentWidth());
                        } else {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->contentWidth() /
                                           cb->contentWidth());
                        }
                    } else if (toValue.isFixed() && !fromValue.isFixed()) {
                        float value;
                        if (oldStyle->boxSizing() ==
                            BoxSizingValue::BorderBoxBoxSizingValue) {
                            value = oldFrame->asFrameBox()->width();
                        } else {
                            value = oldFrame->asFrameBox()->contentWidth();
                        }
                        fromValue = Length(Length::Fixed, value);
                    }

                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::Width,
                        AnimatedValue(fromValue), AnimatedValue(toValue),
                        duration, delay, timingFunction, newWidth);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::Height) && oldFrame &&
            oldFrame->isFrameBox()) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::Height);
            if (!found) {
                auto oldHeight = oldStyle->height();
                auto newHeight = newStyle->height();

                if ((oldHeight.isPercent() || oldHeight.isFixed()) &&
                    (newHeight.isPercent() || newHeight.isFixed())) {
                    auto fromValue = oldStyle->height();
                    auto toValue = newStyle->height();

                    if (toValue.isPercent() && !fromValue.isPercent()) {
                        FrameBox* cb = containingBlock(oldFrame);
                        if (cb->contentWidth() == 0) {
                            fromValue = Length(Length::Percent, 0);
                        } else if (oldStyle->boxSizing() ==
                                   BoxSizingValue::BorderBoxBoxSizingValue) {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->height() /
                                           cb->contentHeight());
                        } else {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->contentHeight() /
                                           cb->contentHeight());
                        }
                    } else if (toValue.isFixed() && !fromValue.isFixed()) {
                        float value;
                        if (oldStyle->boxSizing() ==
                            BoxSizingValue::BorderBoxBoxSizingValue) {
                            value = oldFrame->asFrameBox()->height();
                        } else {
                            value = oldFrame->asFrameBox()->contentHeight();
                        }
                        fromValue = Length(Length::Fixed, value);
                    }

                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::Height,
                        AnimatedValue(fromValue), AnimatedValue(toValue),
                        duration, delay, timingFunction, newHeight);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MinWidth) && oldFrame &&
            oldFrame->isFrameBox()) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MinWidth);
            if (!found) {
                auto oldWidth = oldStyle->minWidth();
                auto newWidth = newStyle->minWidth();

                if ((oldWidth.isPercent() || oldWidth.isFixed()) &&
                    (newWidth.isPercent() || newWidth.isFixed())) {
                    auto fromValue = oldStyle->minWidth();
                    auto toValue = newStyle->minWidth();

                    if (toValue.isPercent() && !fromValue.isPercent()) {
                        FrameBox* cb = containingBlock(oldFrame);
                        if (cb->contentWidth() == 0) {
                            fromValue = Length(Length::Percent, 0);
                        } else if (oldStyle->boxSizing() ==
                                   BoxSizingValue::BorderBoxBoxSizingValue) {
                            fromValue = Length(Length::Percent,
                                               oldFrame->asFrameBox()->width() /
                                                   cb->contentWidth());
                        } else {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->contentWidth() /
                                           cb->contentWidth());
                        }
                    } else if (toValue.isFixed() && !fromValue.isFixed()) {
                        float value;
                        if (oldStyle->boxSizing() ==
                            BoxSizingValue::BorderBoxBoxSizingValue) {
                            value = oldFrame->asFrameBox()->width();
                        } else {
                            value = oldFrame->asFrameBox()->contentWidth();
                        }
                        fromValue = Length(Length::Fixed, value);
                    }

                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::MinWidth,
                        AnimatedValue(fromValue), AnimatedValue(toValue),
                        duration, delay, timingFunction, newWidth);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MinHeight) && oldFrame &&
            oldFrame->isFrameBox()) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MinHeight);
            if (!found) {
                auto oldHeight = oldStyle->minHeight();
                auto newHeight = newStyle->minHeight();

                if ((oldHeight.isPercent() || oldHeight.isFixed()) &&
                    (newHeight.isPercent() || newHeight.isFixed())) {
                    auto fromValue = oldStyle->minHeight();
                    auto toValue = newStyle->minHeight();

                    if (toValue.isPercent() && !fromValue.isPercent()) {
                        FrameBox* cb = containingBlock(oldFrame);
                        if (cb->contentWidth() == 0) {
                            fromValue = Length(Length::Percent, 0);
                        } else if (oldStyle->boxSizing() ==
                                   BoxSizingValue::BorderBoxBoxSizingValue) {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->height() /
                                           cb->contentHeight());
                        } else {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->contentHeight() /
                                           cb->contentHeight());
                        }
                    } else if (toValue.isFixed() && !fromValue.isFixed()) {
                        float value;
                        if (oldStyle->boxSizing() ==
                            BoxSizingValue::BorderBoxBoxSizingValue) {
                            value = oldFrame->asFrameBox()->height();
                        } else {
                            value = oldFrame->asFrameBox()->contentHeight();
                        }
                        fromValue = Length(Length::Fixed, value);
                    }

                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::MinHeight,
                        AnimatedValue(fromValue), AnimatedValue(toValue),
                        duration, delay, timingFunction, newHeight);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MaxWidth) && oldFrame &&
            oldFrame->isFrameBox()) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MaxWidth);
            if (!found) {
                auto oldWidth = oldStyle->maxWidth();
                auto newWidth = newStyle->maxWidth();

                if ((oldWidth.isPercent() || oldWidth.isFixed()) &&
                    (newWidth.isPercent() || newWidth.isFixed())) {
                    auto fromValue = oldStyle->maxWidth();
                    auto toValue = newStyle->maxWidth();

                    if (toValue.isPercent() && !fromValue.isPercent()) {
                        FrameBox* cb = containingBlock(oldFrame);
                        if (cb->contentWidth() == 0) {
                            fromValue = Length(Length::Percent, 0);
                        } else if (oldStyle->boxSizing() ==
                                   BoxSizingValue::BorderBoxBoxSizingValue) {
                            fromValue = Length(Length::Percent,
                                               oldFrame->asFrameBox()->width() /
                                                   cb->contentWidth());
                        } else {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->contentWidth() /
                                           cb->contentWidth());
                        }
                    } else if (toValue.isFixed() && !fromValue.isFixed()) {
                        float value;
                        if (oldStyle->boxSizing() ==
                            BoxSizingValue::BorderBoxBoxSizingValue) {
                            value = oldFrame->asFrameBox()->width();
                        } else {
                            value = oldFrame->asFrameBox()->contentWidth();
                        }
                        fromValue = Length(Length::Fixed, value);
                    }

                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::MaxWidth,
                        AnimatedValue(fromValue), AnimatedValue(toValue),
                        duration, delay, timingFunction, newWidth);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MaxHeight) && oldFrame &&
            oldFrame->isFrameBox()) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MaxHeight);
            if (!found) {
                auto oldHeight = oldStyle->maxHeight();
                auto newHeight = newStyle->maxHeight();

                if ((oldHeight.isPercent() || oldHeight.isFixed()) &&
                    (newHeight.isPercent() || newHeight.isFixed())) {
                    auto fromValue = oldStyle->maxHeight();
                    auto toValue = newStyle->maxHeight();

                    if (toValue.isPercent() && !fromValue.isPercent()) {
                        FrameBox* cb = containingBlock(oldFrame);
                        if (cb->contentWidth() == 0) {
                            fromValue = Length(Length::Percent, 0);
                        } else if (oldStyle->boxSizing() ==
                                   BoxSizingValue::BorderBoxBoxSizingValue) {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->height() /
                                           cb->contentHeight());
                        } else {
                            fromValue =
                                Length(Length::Percent,
                                       oldFrame->asFrameBox()->contentHeight() /
                                           cb->contentHeight());
                        }
                    } else if (toValue.isFixed() && !fromValue.isFixed()) {
                        float value;
                        if (oldStyle->boxSizing() ==
                            BoxSizingValue::BorderBoxBoxSizingValue) {
                            value = oldFrame->asFrameBox()->height();
                        } else {
                            value = oldFrame->asFrameBox()->contentHeight();
                        }
                        fromValue = Length(Length::Fixed, value);
                    }

                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::MaxHeight,
                        AnimatedValue(fromValue), AnimatedValue(toValue),
                        duration, delay, timingFunction, newHeight);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginTop,
                            CSSStyleValuePair::Margin)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MarginTop);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::marginTopToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginTop, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().top());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginRight,
                            CSSStyleValuePair::Margin)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MarginRight);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::marginRightToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginRight, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().right());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginBottom,
                            CSSStyleValuePair::Margin)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MarginBottom);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::marginBottomToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginBottom, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().bottom());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginLeft,
                            CSSStyleValuePair::Margin)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::MarginLeft);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::marginLeftToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginLeft, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().left());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderTop,
                            CSSStyleValuePair::Border)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderTop);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::borderTopToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderTop, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().top().width());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderRight,
                            CSSStyleValuePair::Border)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderRight);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::borderRightToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderRight, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().right().width());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderBottom,
                            CSSStyleValuePair::Border)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderBottom);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::borderBottomToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderBottom, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().bottom().width());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderLeft,
                            CSSStyleValuePair::Border)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::BorderLeft);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::borderLeftToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderLeft, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().left().width());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingTop,
                            CSSStyleValuePair::Padding)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::PaddingTop);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::paddingTopToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingTop, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->padding().top());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingRight,
                            CSSStyleValuePair::Padding)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::PaddingRight);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::paddingRightToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingRight, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->padding().right());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingBottom,
                            CSSStyleValuePair::Padding)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::PaddingBottom);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::paddingBottomToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingBottom,
                    AnimatedValue(v1), AnimatedValue(v2), duration, delay,
                    timingFunction, newStyle->padding().bottom());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingLeft,
                            CSSStyleValuePair::Padding)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::PaddingLeft);
            AnimatedValue v1, v2;
            if (!found && AnimationUtil::paddingLeftToAnimatedValue(
                              oldStyle, newStyle, element, v1, v2)) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingLeft, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->padding().left());
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

#define GEN_SIDE(Side, side)                                                   \
    if (NEED_TRANSITION(CSSStyleValuePair::Side)) {                            \
        bool found =                                                           \
            executor->hasActiveAnimiation(element, CSSStyleValuePair::Side);   \
        AnimatedValue v1, v2;                                                  \
        if (!found && AnimationUtil::lengthToAnimatedValue(oldStyle->side(),   \
                                                           newStyle->side(),   \
                                                           element, v1, v2)) { \
            auto task = new ActiveLengthAnimationTask(                         \
                element, CSSStyleValuePair::Side, AnimatedValue(v1),           \
                AnimatedValue(v2), duration, delay, timingFunction,            \
                newStyle->side());                                             \
            executor->registerAnimation(task, newStyle);                       \
            gotTransition = true;                                              \
        }                                                                      \
    }
        GEN_SIDE(Left, left)
        GEN_SIDE(Right, right)
        GEN_SIDE(Top, top)
        GEN_SIDE(Bottom, bottom)
#undef GEN_SIDE

        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionX,
                            CSSStyleValuePair::BackgroundPosition,
                            CSSStyleValuePair::Background)) {
            if (!oldStyle->hasBlockLikeDisplay() ||
                !newStyle->hasBlockLikeDisplay()) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            for (size_t i = 0; i < layerSize; i++) {
                bool found = executor->hasActiveAnimiation(
                    element, CSSStyleValuePair::BackgroundPositionX);
                AnimatedValue pos1, pos2;
                if (!found && AnimationUtil::backgroundPosXToAnimatedValue(
                                  oldStyle, newStyle, oldPaintingBox, element,
                                  pos1, pos2, i)) {
                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionX,
                        AnimatedValue(pos1), AnimatedValue(pos2), duration,
                        delay, timingFunction, newStyle->backgroundPositionX(i),
                        i);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionY,
                            CSSStyleValuePair::BackgroundPosition,
                            CSSStyleValuePair::Background)) {
            if (!oldStyle->hasBlockLikeDisplay() ||
                !newStyle->hasBlockLikeDisplay()) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            for (size_t i = 0; i < layerSize; i++) {
                bool found = executor->hasActiveAnimiation(
                    element, CSSStyleValuePair::BackgroundPositionY);
                AnimatedValue pos1, pos2;
                if (!found && AnimationUtil::backgroundPosYToAnimatedValue(
                                  oldStyle, newStyle, oldPaintingBox, element,
                                  pos1, pos2, i)) {
                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionY,
                        AnimatedValue(pos1), AnimatedValue(pos2), duration,
                        delay, timingFunction, newStyle->backgroundPositionY(i),
                        i);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        // NOTE background-size should come after background-position
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundSize,
                            CSSStyleValuePair::Background)) {
            if (!oldStyle->hasBlockLikeDisplay() ||
                !newStyle->hasBlockLikeDisplay()) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            for (size_t i = 0; i < layerSize; i++) {
                AnimatedValue size1, size2;
                bool found = executor->hasActiveAnimiation(
                    element, CSSStyleValuePair::BackgroundSize);
                if (!found && AnimationUtil::backgroundSizeToAnimatedValue(
                                  oldStyle, newStyle, oldPaintingBox, element,
                                  size1, size2, i)) {
                    auto task = new ActiveLengthSizeAnimationTask(
                        element, CSSStyleValuePair::BackgroundSize,
                        AnimatedValue(size1), AnimatedValue(size2), duration,
                        delay, timingFunction,
                        newStyle->backgroundSizeLengthValue(i), i);
                    executor->registerAnimation(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::FontSize,
                            CSSStyleValuePair::Font)) {
            bool found = executor->hasActiveAnimiation(
                element, CSSStyleValuePair::FontSize);
            if (!found) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::FontSize,
                    AnimatedValue(
                        Length(Length::Fixed, oldStyle->fixedFontSize())),
                    AnimatedValue(
                        Length(Length::Fixed, newStyle->fixedFontSize())),
                    duration, delay, timingFunction,
                    Length(Length::Fixed, newStyle->fixedFontSize()));
                executor->registerAnimation(task, newStyle);
                gotTransition = true;
            }
        }

        // <- length series

        if (gotTransition) {
            // TODO reduce animation duration here with
            // canceledAnimationProgress
            ret = true;
        }
    }

    return ret;
}
}
