/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include <SkMatrix.h>

#include "core/animation/AnimationTask.h"
#include "core/animation/util/AnimationUtil.h"
#include "core/animation/TimingFunction.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/dom/svg/SVGAnimateMotionElement.h"
#include "core/dom/AnimationEvent.h"
#include "core/dom/TransitionEvent.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/StackingContext.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/style/CalcData.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSProperty.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "core/animation/AnimationApplier.h"
#include "core/animation/TransitionApplier.h"
#include "core/style/FilterFunctions.h"

namespace Starfish {

template <typename T>
static float interpolate(const T from, const T to, double progress,
                         bool isForward = true)
{
    if (isForward) {
        return from + (to - from) * progress;
    } else {
        return from + (to - from) * (1 - progress);
    }
}

static Length interpolateLength(double progress, const Length& from,
                                const Length& to, bool isForward)
{
    Length newLength;
    if ((to.isPercent() || to.isZero()) &&
        (from.isPercent() || from.isZero())) {
        float fromPercent = 0;
        if (!from.isZero()) {
            fromPercent = from.percent();
        }
        float toPercent = 0;
        if (!to.isZero()) {
            toPercent = to.percent();
        }
        newLength = Length(Length::Percent, interpolate(fromPercent, toPercent,
                                                        progress, isForward));
    } else if (from.isAuto() || to.isAuto()) {
        if (progress < 0.5) {
            if (!from.isAuto()) {
                newLength = from;
            } else {
                newLength = Length();
            }
        } else {
            if (!to.isAuto()) {
                newLength = to;
            } else {
                newLength = Length();
            }
        }
    } else {
        float fromFixed = from.fixed();
        float toFixed = to.fixed();
        newLength = Length(Length::Fixed, interpolate(fromFixed, toFixed,
                                                      progress, isForward));
    }

    return newLength;
}

void* ActiveAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ActiveAnimationTask)] = { 0 };
        fillGCDescriptor(obj_bitmap);
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ActiveAnimationTask));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ActiveAnimationTask::ActiveAnimationTask(const ActiveAnimationTaskInit& init)
{
    initialize(init);
}

void ActiveAnimationTask::initialize(const ActiveAnimationTaskInit& init)
{
    m_animationType = init.animationType;
    m_property = init.targetProperty;
    m_targetElement = init.target;
    m_durationMs = init.durationInMs;
    m_startDelayMs = init.delayInMs;
    m_delayMs = init.delayInMs;
    m_playState = init.playState;
    m_fillMode = init.fillMode;
    m_iterationCount = init.iterationCount;
    m_isInDelayedTime = init.delayInMs > 0 ? true : false;
    m_frameSize = init.animatedValues.size();
    m_values.assign(init.animatedValues.begin(), init.animatedValues.end());
    m_offsets.assign(init.offsets.begin(), init.offsets.end());
    m_timingFunctions.assign(init.timingFunctions.begin(),
                             init.timingFunctions.end());
    m_layerIndex = init.layerIndex;

    m_startTimeMs = 0;
    m_gapTimeMs = 0;
    m_iterationStart = 0;
    m_isForward = true;
    m_isRunning = true;
    m_isInForwardsFillMode = false;
    m_didReachedToEnd = false;
    m_frameIdx = 0;

    if (init.animationType == AnimationType::Transition) {
        // Note that this originated from legacy code.
        m_isEveryAnimiatedValueResolved = true;
        STARFISH_ASSERT(m_playState == AnimationPlayStateValue::Running);
        STARFISH_ASSERT(m_fillMode == AnimationFillModeValue::None);
        STARFISH_ASSERT(m_iterationCount == 0);
        STARFISH_ASSERT(m_iterationStart == 0);
        STARFISH_ASSERT(m_frameSize == 2);
        STARFISH_ASSERT(m_offsets.size() == 0);
        STARFISH_ASSERT(init.timingFunctions.size() == 1);
        STARFISH_ASSERT(init.animatedValues.size() == 2);
    } else {
        // Note that this originated from legacy code.
        m_isEveryAnimiatedValueResolved = false;
        STARFISH_ASSERT(init.animatedValues.size() > 1);
        STARFISH_ASSERT(init.offsets.size() > 1);
        STARFISH_ASSERT(init.timingFunctions.size() > 1);
    }

    if (init.animationType == AnimationType::SVGAnimation) {
        STARFISH_ASSERT(init.originAnimationElement.hasValue());
        m_originAnimationElement = init.originAnimationElement;
    }
}

void ActiveAnimationTask::step(uint64_t currentTickCount, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    if (!m_startTimeMs) {
        m_startTimeMs = currentTickCount;
    }

    if (isTransition()) {
        double f = fraction(currentTickCount);
        execute(computeProgress(f), style);
    } else {
        if (!m_isEveryAnimiatedValueResolved) {
            return;
        }

        if (m_isInDelayedTime) {
            if (m_startTimeMs + m_delayMs > currentTickCount) {
                return;
            } else {
                m_startTimeMs += m_delayMs;
                m_delayMs = 0;
                m_isInDelayedTime = false;
            }
        }

        double f = fraction(currentTickCount);
        execute(computeProgress(f), style);

        if (m_isInForwardsFillMode) {
            return;
        }

        auto frameIdxBefore = m_frameIdx;
        if (f >= 1.0 && m_isForward) {
            m_frameIdx++;
            if (m_frameIdx == m_frameSize - 1) {
                m_frameIdx = 0;
                m_startTimeMs = m_startTimeMs + m_durationMs;
                m_didReachedToEnd = true;
            }
        } else if (f <= 0.0 && !m_isForward) {
            m_frameIdx--;
            if (m_frameIdx == 0) {
                m_frameIdx = m_frameSize - 1;
                m_startTimeMs = m_startTimeMs + m_durationMs;
                m_didReachedToEnd = true;
            }
        }
        if (frameIdxBefore != m_frameIdx) {
            didAnimationFrameChanged();
        }

        if (!std::isinf(m_iterationCount) && m_gapTimeMs == 0 &&
            m_fillMode == AnimationFillModeValue::Forwards) {
            return;
        }
    }
}

double ActiveAnimationTask::fraction(uint64_t tickCount) const
{
    STARFISH_ASSERT(m_durationMs != 0);
    if (m_isInForwardsFillMode) {
        return 1.0f;
    }

    if (!m_isRunning) {
        double result = m_gapTimeMs / static_cast<double>(m_durationMs);
        return std::min(result, 1.0);
    }

    if (!m_startTimeMs) {
        return 0;
    }

    if (tickCount < (m_startTimeMs + m_delayMs)) {
        return 0;
    }

    uint64_t timeDiff = tickCount - (m_startTimeMs + m_delayMs);
    double result = timeDiff / static_cast<double>(m_durationMs);

    return std::min(result, 1.0);
}

uint64_t ActiveAnimationTask::remainTime(uint64_t tickCount) const
{
    if (m_isInForwardsFillMode) {
        return 0;
    }

    if (!m_isRunning) {
        return m_durationMs - m_gapTimeMs;
    }

    if (!m_startTimeMs) {
        return m_durationMs;
    }

    if (tickCount < (m_startTimeMs + m_delayMs)) {
        return m_durationMs;
    }

    uint64_t timeDiff = tickCount - (m_startTimeMs + m_delayMs);

    if (timeDiff > m_durationMs) {
        return 0;
    }

    return m_durationMs - timeDiff;
}

void ActiveAnimationTask::setIsForward(bool isForward)
{
    if (m_isForward && !isForward) {
        m_frameIdx = m_frameSize - 1;
    } else if (!m_isForward && isForward) {
        m_frameIdx = 0;
    }
    m_isForward = isForward;
}

void ActiveAnimationTask::fireTransitionStartEvent()
{
    // STARFISH_LOG_INFO("element %p property %s transitionStart",
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

void ActiveAnimationTask::fireTransitionEndEvent()
{
    // STARFISH_LOG_INFO("element %p property %s transitionEnd",
    // m_targetElement, CSSPropertyHelper::toString(m_property));
    TransitionEventInit init;
    init.setPropertyName(CSSPropertyHelper::toGCString(m_property));
    init.setBubbles(true);
    init.setCancelable(true);
    // TODO add more information to init
    TransitionEvent* event =
        new TransitionEvent(m_targetElement->executionContext(),
                            m_targetElement->starfish()
                                ->staticStrings()
                                ->m_transitionend.localName(),
                            init);
    m_targetElement->dispatchEventIdleTimeByUA(event);
}

void ActiveAnimationTask::fireTransitionCancelEvent()
{
    // STARFISH_LOG_INFO("element %p property %s transitionCancel",
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

AnimatedValue* ActiveAnimationTask::currentAnimatedFromValue()
{
    return m_values[m_frameIdx];
}

AnimatedValue* ActiveAnimationTask::currentAnimatedToValue()
{
    if (m_isForward) {
        return m_values[m_frameIdx + 1];
    } else {
        return m_values[m_frameIdx - 1];
    }
}

TimingFunction* ActiveAnimationTask::currentTimingFunction()
{
    return m_timingFunctions[m_frameIdx];
}

bool ActiveAnimationTask::needsContinuousRendering(uint64_t tick)
{
    bool b = !!m_targetElement->frame();
    if (b) {
        auto f = m_targetElement->frame();
        f = f->parent();
        while (f) {
            if (f->style() && f->style()->opacity() == 0) {
                return false;
            }
            f = f->layoutParent();
        }
        return true;
    }
    return false;
}

double ActiveAnimationTask::computeProgress(double& fraction)
{
    STARFISH_ASSERT(fraction >= 0.0);
    STARFISH_ASSERT(fraction <= 1.0);

    if (!isTransition()) {
        if (m_isForward) {
            fraction = (fraction - m_offsets[m_frameIdx]) /
                       (m_offsets[m_frameIdx + 1] - m_offsets[m_frameIdx]);
        } else {
            fraction = (1 - fraction - m_offsets[m_frameIdx - 1]) /
                       (m_offsets[m_frameIdx] - m_offsets[m_frameIdx - 1]);
        }
        fraction = fraction > 1 ? 1 : fraction < 0 ? 0 : fraction;
    }
    return currentTimingFunction()->getValue(fraction);
}

ActiveOpacityAnimationTask::ActiveOpacityAnimationTask(
    const ActiveAnimationTaskInit& init)
    : ActiveAnimationTask(init)
{
    if (init.animationType != AnimationType::Transition) {
        // Note that this originated from legacy code.
        m_isEveryAnimiatedValueResolved = true;
    }
}

void ActiveOpacityAnimationTask::execute(double progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    float from = currentAnimatedFromValue()->getFloat();
    float to = currentAnimatedToValue()->getFloat();
    float value = 0;
    if (m_isForward) {
        value = from * (1 - progress) + to * progress;
    } else {
        value = from * progress + to * (1 - progress);
    }
    style->setOpacity(value);
}

bool ActiveOpacityAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    if (newStyle->opacity() != currentAnimatedToValue()->getFloat()) {
        return false;
    }
    return true;
}

void ActiveOpacityAnimationTask::attachToElement()
{
    m_targetElement->markRunningOpacityAnimation();
}

void ActiveOpacityAnimationTask::detachFromElement()
{
    m_targetElement->clearRunningOpacityAnimation();
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
    ret.angle = UnitHelper::convertFromRadToDeg(angle);
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
    float angle = UnitHelper::convertFromDegToRad(decomposed.angle);
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
    const ActiveAnimationTaskInit& init,
    Optional<StyleTransformDataGroup*> originalToValue)
    : ActiveAnimationTask(init)
{
    if (init.animationType == AnimationType::Transition) {
        // Note that this originated from legacy code.
        // But changed to use the clone method instead of direct copy.
        if (originalToValue.hasValue()) {
            m_originalTransformValue = originalToValue.getValue()->clone();
        }
        removePercentValuesFromTransform();
        resolveTransformValues();
    } else {
        m_isEveryAnimiatedValueResolved = false;
    }
}

bool ActiveTransformAnimationTask::needsDecomposing(
    StyleTransformDataGroup* from, StyleTransformDataGroup* to)
{
    if (!m_targetElement->frame()->isTransformable()) {
        return true;
    }

    if (from->size() == 0 || to->size() == 0) {
        return false;
    }

    if (from->size() != to->size()) {
        return true;
    }

    size_t size = from->size();

    for (size_t i = 0; i < size; i++) {
        if (from->at(i).type() != to->at(i).type()) {
            return true;
        }
    }

    return false;
}

void ActiveTransformAnimationTask::didAnimationFrameChanged()
{
    ActiveAnimationTask::didAnimationFrameChanged();
    resolveTransformValues();
}

void ActiveTransformAnimationTask::removePercentValuesFromTransform()
{
    Frame* frm = m_targetElement->frame();
    if (frm->isTransformable()) {
        for (size_t i = 0; i < m_values.size(); i++) {
            STARFISH_ASSERT(m_values[i]->isTransformData());
            auto newTransformData = m_values[i]->getTransformData()->clone();

            // remove percent values from transform
            for (size_t j = 0; j < newTransformData->size(); j++) {
                auto s = newTransformData->at(j);
                if (s.type() == StyleTransformData::Translate) {
                    Length w(Length::Fixed,
                             s.translate()->tx().specifiedValue(
                                 frm->asFrameBox()->width(), m_targetElement));
                    Length h(Length::Fixed,
                             s.translate()->ty().specifiedValue(
                                 frm->asFrameBox()->height(), m_targetElement));
                    s.setTranslate(w, h);
                }
            }

            new (m_values[i]) AnimatedValue(newTransformData);
        }
    }
}

void ActiveTransformAnimationTask::resolveTransformValues()
{
    Frame* frm = m_targetElement->frame();
    if (!frm) {
        return;
    }
    m_shouldUseDecomposing =
        needsDecomposing(currentAnimatedFromValue()->getTransformData(),
                         currentAnimatedToValue()->getTransformData());

    StyleTransformDataGroup* fromTransfromStyle =
        currentAnimatedFromValue()->getTransformData();
    StyleTransformDataGroup* toTransfromStyle =
        currentAnimatedToValue()->getTransformData();

    if (frm->isTransformable()) {
        m_decomposedFrom = decomposing2DMatrix(ComputedStyle::transformToMatrix(
            fromTransfromStyle, frm->asFrameBox()->width(),
            frm->asFrameBox()->height(), frm));
        m_decomposedTo = decomposing2DMatrix(ComputedStyle::transformToMatrix(
            toTransfromStyle, frm->asFrameBox()->width(),
            frm->asFrameBox()->height(), frm));
    } else {
        m_decomposedFrom = m_decomposedTo = decomposing2DMatrix(SkMatrix::I());
    }
    matrixInterpolationPreprocessing(m_decomposedFrom, m_decomposedTo);

    if (!m_shouldUseDecomposing) {
        STARFISH_ASSERT(frm->isTransformable());

        // we need to clone transform values for keeping original values
        if (fromTransfromStyle->size() != 0 && toTransfromStyle->size() == 0) {
            toTransfromStyle =
                fromTransfromStyle->extractTransformFunctionsWithZeroValues();
        } else if (fromTransfromStyle->size() == 0 &&
                   toTransfromStyle->size() != 0) {
            fromTransfromStyle =
                toTransfromStyle->extractTransformFunctionsWithZeroValues();
        }

        STARFISH_ASSERT(fromTransfromStyle->size() == toTransfromStyle->size());

        m_fromTransformValue = fromTransfromStyle;
        m_toTransformValue = toTransfromStyle;
    }
}

void ActiveTransformAnimationTask::resolveUnresolvedAnimatedValues()
{
    if (!m_targetElement->frame()) {
        // Resolve it in the next tick.
        return;
    }

    if (!m_isEveryAnimiatedValueResolved) {
        removePercentValuesFromTransform();
        resolveTransformValues();
    }

    ActiveAnimationTask::resolveUnresolvedAnimatedValues();
}

void* ActiveTransformAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveTransformAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ActiveTransformAnimationTask)] = {
            0
        };
        fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(ActiveTransformAnimationTask));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ActiveTransformAnimationTask::execute(double progress,
                                           ComputedStyle* style)
{
    Element* current = targetElement();
    auto transforms = style->rareComputedStyleData()->transforms();

    if (m_shouldUseDecomposing) {
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
    } else {
        auto newTransform = new StyleTransformDataGroup();
        StyleTransformDataGroup* a = m_fromTransformValue;
        StyleTransformDataGroup* b = m_toTransformValue;

        for (size_t i = 0; i < a->size(); i++) {
            auto newData = a->at(i).clone();
            auto aData = a->at(i);
            auto bData = b->at(i);
            if (aData.type() == StyleTransformData::Matrix) {
                newData.setMatrix(aData.matrix()->a() * (1 - progress) +
                                      bData.matrix()->a() * progress,
                                  aData.matrix()->b() * (1 - progress) +
                                      bData.matrix()->b() * progress,
                                  aData.matrix()->c() * (1 - progress) +
                                      bData.matrix()->c() * progress,
                                  aData.matrix()->d() * (1 - progress) +
                                      bData.matrix()->d() * progress,
                                  aData.matrix()->e() * (1 - progress) +
                                      bData.matrix()->e() * progress,
                                  aData.matrix()->f() * (1 - progress) +
                                      bData.matrix()->f() * progress);
            } else if (aData.type() == StyleTransformData::Translate) {
                newData.setTranslate(
                    Length(Length::Fixed,
                           aData.translate()->tx().fixed() * (1 - progress) +
                               bData.translate()->tx().fixed() * progress),
                    Length(Length::Fixed,
                           aData.translate()->ty().fixed() * (1 - progress) +
                               bData.translate()->ty().fixed() * progress));
            } else if (aData.type() == StyleTransformData::Rotate) {
                double angle = aData.rotate()->angle() * (1 - progress) +
                               bData.rotate()->angle() * progress;
                Length cx =
                    interpolateLength(progress, aData.rotate()->cx(),
                                      bData.rotate()->cx(), m_isForward);
                Length cy =
                    interpolateLength(progress, aData.rotate()->cy(),
                                      bData.rotate()->cy(), m_isForward);
                newData.setRotate(angle, cx, cy);
            } else if (aData.type() == StyleTransformData::Scale) {
                newData.setScale(aData.scale()->x() * (1 - progress) +
                                     bData.scale()->x() * progress,
                                 aData.scale()->y() * (1 - progress) +
                                     bData.scale()->y() * progress);
            } else if (aData.type() == StyleTransformData::Skew) {
                newData.setSkew(aData.skew()->angleX() * (1 - progress) +
                                    bData.skew()->angleX() * progress,
                                aData.skew()->angleY() * (1 - progress) +
                                    bData.skew()->angleY() * progress);
            } else {
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }

            newTransform->append(newData);
        }

        style->setTransform(newTransform);
    }
}

bool ActiveTransformAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    if (newStyle->transforms() == nullptr) {
        if (m_originalTransformValue == nullptr ||
            m_originalTransformValue->size() == 0) {
            return true;
        }
        return false;
    }
    if (m_originalTransformValue == nullptr) {
        if (newStyle->transforms() == nullptr ||
            newStyle->transforms()->size() == 0) {
            return true;
        }
        return false;
    }
    if (*newStyle->transforms() != *m_originalTransformValue) {
        return false;
    }
    return true;
}

void ActiveTransformAnimationTask::attachToElement()
{
    m_targetElement->markRunningTransformAnimation();
}

void ActiveTransformAnimationTask::detachFromElement()
{
    m_targetElement->clearRunningTransformAnimation();
}

ActiveTransformOriginAnimationTask::ActiveTransformOriginAnimationTask(
    const ActiveAnimationTaskInit& init)
    : ActiveAnimationTask(init)
{
}

void ActiveTransformOriginAnimationTask::execute(double progress,
                                                 ComputedStyle* style)
{
    StyleTransformOrigin* from =
        currentAnimatedFromValue()->getTrasnformOriginData();
    StyleTransformOrigin* to =
        currentAnimatedToValue()->getTrasnformOriginData();
    TransformOriginData* fromOrigin = from->originValue();
    TransformOriginData* toOrigin = to->originValue();

    Length fromValues[3];
    fromValues[0] = fromOrigin->getXAxis();
    fromValues[1] = fromOrigin->getYAxis();
    fromValues[2] = fromOrigin->getZAxis();

    Length toValues[3];
    toValues[0] = toOrigin->getXAxis();
    toValues[1] = toOrigin->getYAxis();
    toValues[2] = toOrigin->getZAxis();

    Length newValues[3];
    for (size_t i = 0; i < 3; i++) {
        newValues[i] = interpolateLength(progress, fromValues[i], toValues[i],
                                         m_isForward);
    }

    style->ensureTransformOrigin()->originValue()->setData(
        newValues[0], newValues[1], newValues[2]);
}

ActiveColorAnimationTask::ActiveColorAnimationTask(
    const ActiveAnimationTaskInit& init)
    : ActiveAnimationTask(init)
{
    if (init.animationType != AnimationType::Transition) {
        // Note that this originated from legacy code.
        m_isEveryAnimiatedValueResolved = true;
    }
}

void ActiveColorAnimationTask::execute(double progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    Unit::Color from = currentAnimatedFromValue()->getColor();
    Unit::Color to = currentAnimatedToValue()->getColor();

    unsigned char r, g, b, a;
    if (m_isForward) {
        r = from.r() * (1 - progress) + to.r() * progress;
        g = from.g() * (1 - progress) + to.g() * progress;
        b = from.b() * (1 - progress) + to.b() * progress;
        a = from.a() * (1 - progress) + to.a() * progress;
    } else {
        r = from.r() * progress + to.r() * (1 - progress);
        g = from.g() * progress + to.g() * (1 - progress);
        b = from.b() * progress + to.b() * (1 - progress);
        a = from.a() * progress + to.a() * (1 - progress);
    }

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
    } else if (m_property == CSSStyleValuePair::KeyKind::Fill) {
        style->setFill(new StylePaintData(Unit::Color(r, g, b, a)));
    } else {
        STARFISH_UNSUPPORTED("css animation property: %d", m_property);
    }
}

bool ActiveColorAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        return newStyle->backgroundColor() ==
               currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottomColor) {
        return newStyle->border().bottom().color() ==
               currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeftColor) {
        return newStyle->border().left().color() ==
               currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRightColor) {
        return newStyle->border().right().color() ==
               currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTopColor) {
        return newStyle->border().top().color() ==
               currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::Color) {
        return newStyle->color() == currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::CaretColor) {
        return newStyle->caretColor() == currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::OutlineColor) {
        return newStyle->outlineColor() == currentAnimatedToValue()->getColor();
    } else if (m_property == CSSStyleValuePair::KeyKind::TextDecorationColor) {
        return newStyle->textDecorationColor() ==
               currentAnimatedToValue()->getColor();
    } else {
        STARFISH_UNSUPPORTED("css animation property: %d", m_property);
    }
    return false;
}

bool ActiveColorAnimationTask::isKindOfTransitionProperty(
    CSSStyleValuePair::KeyKind key)
{
    switch (m_property) {
    case CSSStyleValuePair::KeyKind::BackgroundColor:
        return key == CSSStyleValuePair::KeyKind::BackgroundColor ||
               key == CSSStyleValuePair::KeyKind::Background;
    case CSSStyleValuePair::KeyKind::BorderBottomColor:
        return key == CSSStyleValuePair::KeyKind::BorderBottomColor ||
               key == CSSStyleValuePair::KeyKind::BorderColor ||
               key == CSSStyleValuePair::KeyKind::BorderBottom ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::BorderLeftColor:
        return key == CSSStyleValuePair::KeyKind::BorderLeftColor ||
               key == CSSStyleValuePair::KeyKind::BorderColor ||
               key == CSSStyleValuePair::KeyKind::BorderLeft ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::BorderRightColor:
        return key == CSSStyleValuePair::KeyKind::BorderRightColor ||
               key == CSSStyleValuePair::KeyKind::BorderColor ||
               key == CSSStyleValuePair::KeyKind::BorderRight ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::BorderTopColor:
        return key == CSSStyleValuePair::KeyKind::BorderTopColor ||
               key == CSSStyleValuePair::KeyKind::BorderColor ||
               key == CSSStyleValuePair::KeyKind::BorderTop ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::TextDecorationColor:
        return key == CSSStyleValuePair::KeyKind::TextDecorationColor ||
               key == CSSStyleValuePair::KeyKind::TextDecoration;
    default:
        return key == m_property;
    }

    return false;
}

ActiveLengthAnimationTask::ActiveLengthAnimationTask(
    const ActiveAnimationTaskInit& init, Optional<Length> originalToValue)
    : ActiveAnimationTask(init)
{
    if (init.animationType == AnimationType::Transition) {
        // Note that this originated from legacy code.
        m_originalToValue = originalToValue;
    }
}

void* ActiveLengthAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveLengthAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ActiveLengthAnimationTask)] = { 0 };
        fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(ActiveLengthAnimationTask));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ActiveLengthAnimationTask::resolveUnresolvedAnimatedValues()
{
    if (!m_isEveryAnimiatedValueResolved) {
        bool isEveryValueHasPercent = true;
        for (size_t i = 0; i < m_values.size(); i++) {
            if (!m_values[i]->getLength().isPercent() &&
                !m_values[i]->getLength().isZero()) {
                isEveryValueHasPercent = false;
                break;
            }
        }

        if (!m_targetElement->frame()) {
            // Resolve it in the next tick.
            return;
        }

        if (!isEveryValueHasPercent) {
            Frame* frm = m_targetElement->frame();
            FrameBox* cb = containingBlock(frm);

            LayoutUnit parentLength;
            bool canUsePercent = false;
            bool isValueKindDependsOnParentFixedHeight = false;
            bool hasParentHeightFixedHeight = false;

            if (cb != nullptr) {
                switch (m_property) {
                case CSSStyleValuePair::Width:
                case CSSStyleValuePair::MaxWidth:
                case CSSStyleValuePair::MinWidth:
                case CSSStyleValuePair::MarginTop:
                case CSSStyleValuePair::MarginRight:
                case CSSStyleValuePair::MarginBottom:
                case CSSStyleValuePair::MarginLeft:
                case CSSStyleValuePair::BorderTopWidth:
                case CSSStyleValuePair::BorderRightWidth:
                case CSSStyleValuePair::BorderBottomWidth:
                case CSSStyleValuePair::BorderLeftWidth:
                case CSSStyleValuePair::PaddingTop:
                case CSSStyleValuePair::PaddingRight:
                case CSSStyleValuePair::PaddingBottom:
                case CSSStyleValuePair::PaddingLeft:
                    parentLength = cb->contentWidth();
                    break;
                case CSSStyleValuePair::Left:
                case CSSStyleValuePair::Right:
                    parentLength = cb->contentWidth();
                    break;
                case CSSStyleValuePair::Top:
                case CSSStyleValuePair::Bottom:
                    parentLength = cb->contentHeight();
                    break;
                case CSSStyleValuePair::Height:
                case CSSStyleValuePair::MaxHeight:
                case CSSStyleValuePair::MinHeight:
                    isValueKindDependsOnParentFixedHeight = true;
                    hasParentHeightFixedHeight =
                        LayoutContext::parentHasFixedHeight(frm);
                    parentLength = cb->contentHeight();
                    break;
                case CSSStyleValuePair::BackgroundPositionX: {
                    Unit::Size posSize, imgSize;
                    AnimationUtil::calculateBackgroundBaseData(
                        m_targetElement->frame()->asFrameBox(),
                        m_targetElement->style(), layerIndex(), posSize,
                        imgSize);
                    if (imgSize.width() == 0.0 || imgSize.height() == 0.0) {
                        return;
                    }
                    parentLength = posSize.width() - imgSize.width();
                    break;
                }
                case CSSStyleValuePair::BackgroundPositionY: {
                    Unit::Size posSize, imgSize;
                    AnimationUtil::calculateBackgroundBaseData(
                        m_targetElement->frame()->asFrameBox(),
                        m_targetElement->style(), layerIndex(), posSize,
                        imgSize);
                    if (imgSize.width() == 0.0 || imgSize.height() == 0.0) {
                        return;
                    }
                    parentLength = posSize.height() - imgSize.height();
                    break;
                }
                case CSSStyleValuePair::FontSize:
                    break;
                case CSSStyleValuePair::X:
                case CSSStyleValuePair::CX:
                case CSSStyleValuePair::RX: {
                    if (frm->isFrameSVGBox()) {
                        auto viewport = frm->asFrameSVGBox()->viewport();
                        parentLength = viewport.width();
                    }
                } break;

                case CSSStyleValuePair::Y:
                case CSSStyleValuePair::CY:
                case CSSStyleValuePair::RY: {
                    if (frm->isFrameSVGBox()) {
                        auto viewport = frm->asFrameSVGBox()->viewport();
                        parentLength = viewport.height();
                    }
                } break;
                default:
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            } else {
                switch (m_property) {
                case CSSStyleValuePair::Width:
                case CSSStyleValuePair::MaxWidth:
                case CSSStyleValuePair::MinWidth:
                case CSSStyleValuePair::MarginTop:
                case CSSStyleValuePair::MarginRight:
                case CSSStyleValuePair::MarginBottom:
                case CSSStyleValuePair::MarginLeft:
                case CSSStyleValuePair::BorderTop:
                case CSSStyleValuePair::BorderRight:
                case CSSStyleValuePair::BorderBottom:
                case CSSStyleValuePair::BorderLeft:
                case CSSStyleValuePair::PaddingTop:
                case CSSStyleValuePair::PaddingRight:
                case CSSStyleValuePair::PaddingBottom:
                case CSSStyleValuePair::PaddingLeft:
                case CSSStyleValuePair::Left:
                case CSSStyleValuePair::Right:
                case CSSStyleValuePair::Top:
                case CSSStyleValuePair::Bottom:
                    break;
                case CSSStyleValuePair::Height:
                case CSSStyleValuePair::MaxHeight:
                case CSSStyleValuePair::MinHeight:
                    isValueKindDependsOnParentFixedHeight = true;
                    hasParentHeightFixedHeight = false;
                    break;
                default:
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }

            // A = height-kind with parent fixed height
            // B = height-kind without parent fixed height
            // value    | width-kind       | A                | B
            // auto     | auto             | auto             | auto
            // fixed    | fixed            | fixed            | fixed
            // percent  | convert as fixed | convert as fixed | invalid value

            for (size_t i = 0; i < m_values.size(); i++) {
                STARFISH_ASSERT(m_values[i]->isLength());
                if (m_values[i]->getLength().hasPercent()) {
                    if (isValueKindDependsOnParentFixedHeight) {
                        if (hasParentHeightFixedHeight) {
                            new (m_values[i]) AnimatedValue(
                                Length(Length::Fixed,
                                       m_values[i]->getLength().specifiedValue(
                                           parentLength, m_targetElement)));
                        } else {
                            new (m_values[i]) AnimatedValue(Length());
                        }
                    } else {
                        new (m_values[i]) AnimatedValue(
                            Length(Length::Fixed,
                                   m_values[i]->getLength().specifiedValue(
                                       parentLength, m_targetElement)));
                    }

                } else if (!m_values[i]->getLength().isAuto()) {
                    new (m_values[i]) AnimatedValue(Length(
                        Length::Fixed, m_values[i]->getLength().specifiedValue(
                                           parentLength, m_targetElement)));
                }

                STARFISH_ASSERT(!m_values[i]->getLength().hasPercent());
            }
        }
    }

    ActiveAnimationTask::resolveUnresolvedAnimatedValues();
}

static Length computeLengthAnimationValue(AnimatedValue* fromValue,
                                          AnimatedValue* toValue,
                                          float progress, bool isForward)
{
    Length newLength;
    if (!fromValue->isLength() || !toValue->isLength()) {
        // newLength remains as auto
    } else if (fromValue->getLength().isAuto() ||
               toValue->getLength().isAuto()) {
        if (progress < 0.5) {
            if (fromValue->isLength()) {
                newLength = fromValue->getLength();
            } else {
                newLength = Length();
            }
        } else {
            if (toValue->isLength()) {
                newLength = toValue->getLength();
            } else {
                newLength = Length();
            }
        }
    } else {
        if ((toValue->getLength().isPercent() ||
             toValue->getLength().isZero()) &&
            (fromValue->getLength().isPercent() ||
             fromValue->getLength().isZero())) {
            float fromPercent = 0;
            if (!fromValue->getLength().isZero()) {
                fromPercent = fromValue->getLength().percent();
            }
            float toPercent = 0;
            if (!toValue->getLength().isZero()) {
                toPercent = toValue->getLength().percent();
            }
            newLength =
                Length(Length::Percent, interpolate(fromPercent, toPercent,
                                                    progress, isForward));
        } else {
            float fromFixed = fromValue->getLength().fixed();
            float toFixed = toValue->getLength().fixed();
            newLength = Length(Length::Fixed, interpolate(fromFixed, toFixed,
                                                          progress, isForward));
        }
    }
    return newLength;
}

void ActiveLengthAnimationTask::execute(double progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    Length newLength;

    if (!m_isEveryAnimiatedValueResolved) {
        newLength = currentAnimatedFromValue()->getLength();
    } else {
        newLength = computeLengthAnimationValue(currentAnimatedFromValue(),
                                                currentAnimatedToValue(),
                                                progress, m_isForward);
    }

    switch (m_property) {
    case CSSStyleValuePair::KeyKind::Width:
        style->setWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::Height:
        style->setHeight(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MarginTop:
        style->setMarginTop(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MarginRight:
        style->setMarginRight(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MarginBottom:
        style->setMarginBottom(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MarginLeft:
        style->setMarginLeft(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MinWidth:
        style->setMinWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MinHeight:
        style->setMinHeight(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MaxWidth:
        style->setMaxWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::MaxHeight:
        style->setMaxHeight(newLength);
        break;
    case CSSStyleValuePair::KeyKind::PaddingTop:
        style->setPaddingTop(newLength);
        break;
    case CSSStyleValuePair::KeyKind::PaddingRight:
        style->setPaddingRight(newLength);
        break;
    case CSSStyleValuePair::KeyKind::PaddingBottom:
        style->setPaddingBottom(newLength);
        break;
    case CSSStyleValuePair::KeyKind::PaddingLeft:
        style->setPaddingLeft(newLength);
        break;
    case CSSStyleValuePair::KeyKind::BorderTopWidth:
        style->setBorderTopWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::BorderRightWidth:
        style->setBorderRightWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::BorderBottomWidth:
        style->setBorderBottomWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::BorderLeftWidth:
        style->setBorderLeftWidth(newLength);
        break;
    case CSSStyleValuePair::KeyKind::Left:
        style->setLeft(newLength);
        break;
    case CSSStyleValuePair::KeyKind::Top:
        style->setTop(newLength);
        break;
    case CSSStyleValuePair::KeyKind::Right:
        style->setRight(newLength);
        break;
    case CSSStyleValuePair::KeyKind::Bottom:
        style->setBottom(newLength);
        break;
    case CSSStyleValuePair::KeyKind::BackgroundPositionX:
        style->setBackgroundPositionX(newLength, layerIndex());
        break;
    case CSSStyleValuePair::KeyKind::BackgroundPositionY:
        style->setBackgroundPositionY(newLength, layerIndex());
        break;
    case CSSStyleValuePair::KeyKind::FontSize:
        style->setFontSize(newLength);
        style->loadFont(m_targetElement);
        break;
    case CSSStyleValuePair::KeyKind::X:
        style->setX(newLength);
        break;
    case CSSStyleValuePair::KeyKind::Y:
        style->setY(newLength);
        break;
    case CSSStyleValuePair::KeyKind::CX:
        style->setCX(newLength);
        break;
    case CSSStyleValuePair::KeyKind::CY:
        style->setCY(newLength);
        break;
    case CSSStyleValuePair::KeyKind::RX:
        style->setRX(newLength);
        break;
    case CSSStyleValuePair::KeyKind::RY:
        style->setRY(newLength);
        break;
    default:
        break;
    }
}

bool ActiveLengthAnimationTask::isKindOfTransitionProperty(
    CSSStyleValuePair::KeyKind key)
{
    switch (m_property) {
    case CSSStyleValuePair::KeyKind::MarginTop:
        return key == CSSStyleValuePair::KeyKind::MarginTop ||
               key == CSSStyleValuePair::KeyKind::Margin;
    case CSSStyleValuePair::KeyKind::MarginRight:
        return key == CSSStyleValuePair::KeyKind::MarginRight ||
               key == CSSStyleValuePair::KeyKind::Margin;
    case CSSStyleValuePair::KeyKind::MarginBottom:
        return key == CSSStyleValuePair::KeyKind::MarginBottom ||
               key == CSSStyleValuePair::KeyKind::Margin;
    case CSSStyleValuePair::KeyKind::MarginLeft:
        return key == CSSStyleValuePair::KeyKind::MarginLeft ||
               key == CSSStyleValuePair::KeyKind::Margin;
    case CSSStyleValuePair::KeyKind::BorderTopWidth:
        return key == CSSStyleValuePair::KeyKind::BorderTopWidth ||
               key == CSSStyleValuePair::KeyKind::BorderTop ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::BorderRightWidth:
        return key == CSSStyleValuePair::KeyKind::BorderRightWidth ||
               key == CSSStyleValuePair::KeyKind::BorderRight ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::BorderBottomWidth:
        return key == CSSStyleValuePair::KeyKind::BorderBottomWidth ||
               key == CSSStyleValuePair::KeyKind::BorderBottom ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::BorderLeftWidth:
        return key == CSSStyleValuePair::KeyKind::BorderLeftWidth ||
               key == CSSStyleValuePair::KeyKind::BorderLeft ||
               key == CSSStyleValuePair::KeyKind::Border;
    case CSSStyleValuePair::KeyKind::PaddingTop:
        return key == CSSStyleValuePair::KeyKind::PaddingTop ||
               key == CSSStyleValuePair::KeyKind::Padding;
    case CSSStyleValuePair::KeyKind::PaddingRight:
        return key == CSSStyleValuePair::KeyKind::PaddingRight ||
               key == CSSStyleValuePair::KeyKind::Padding;
    case CSSStyleValuePair::KeyKind::PaddingBottom:
        return key == CSSStyleValuePair::KeyKind::PaddingBottom ||
               key == CSSStyleValuePair::KeyKind::Padding;
    case CSSStyleValuePair::KeyKind::PaddingLeft:
        return key == CSSStyleValuePair::KeyKind::PaddingLeft ||
               key == CSSStyleValuePair::KeyKind::Padding;
    case CSSStyleValuePair::KeyKind::BackgroundPositionX:
        return key == CSSStyleValuePair::KeyKind::BackgroundPositionX ||
               key == CSSStyleValuePair::KeyKind::BackgroundPosition ||
               key == CSSStyleValuePair::KeyKind::Background;
    case CSSStyleValuePair::KeyKind::BackgroundPositionY:
        return key == CSSStyleValuePair::KeyKind::BackgroundPositionY ||
               key == CSSStyleValuePair::KeyKind::BackgroundPosition ||
               key == CSSStyleValuePair::KeyKind::Background;
    case CSSStyleValuePair::KeyKind::FontSize:
        return key == CSSStyleValuePair::KeyKind::FontSize ||
               key == CSSStyleValuePair::KeyKind::Font;
    default:
        return m_property == key;
    }

    return false;
}

bool ActiveLengthAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    STARFISH_ASSERT(m_originalToValue.hasValue());

    switch (m_property) {
    case CSSStyleValuePair::KeyKind::Width:
        return m_originalToValue.getValue() == newStyle->width();
    case CSSStyleValuePair::KeyKind::Height:
        return m_originalToValue.getValue() == newStyle->height();
    case CSSStyleValuePair::KeyKind::MarginTop:
        return m_originalToValue.getValue() == newStyle->margin().top();
    case CSSStyleValuePair::KeyKind::MarginRight:
        return m_originalToValue.getValue() == newStyle->margin().right();
    case CSSStyleValuePair::KeyKind::MarginBottom:
        return m_originalToValue.getValue() == newStyle->margin().bottom();
    case CSSStyleValuePair::KeyKind::MarginLeft:
        return m_originalToValue.getValue() == newStyle->margin().left();
    case CSSStyleValuePair::KeyKind::MinWidth:
        return m_originalToValue.getValue() == newStyle->minWidth();
    case CSSStyleValuePair::KeyKind::MinHeight:
        return m_originalToValue.getValue() == newStyle->minHeight();
    case CSSStyleValuePair::KeyKind::MaxWidth:
        return m_originalToValue.getValue() == newStyle->maxWidth();
    case CSSStyleValuePair::KeyKind::MaxHeight:
        return m_originalToValue.getValue() == newStyle->maxHeight();
    case CSSStyleValuePair::KeyKind::BorderTopWidth:
        return m_originalToValue.getValue() == newStyle->border().top().width();
    case CSSStyleValuePair::KeyKind::BorderRightWidth:
        return m_originalToValue.getValue() ==
               newStyle->border().right().width();
    case CSSStyleValuePair::KeyKind::BorderBottomWidth:
        return m_originalToValue.getValue() ==
               newStyle->border().bottom().width();
    case CSSStyleValuePair::KeyKind::BorderLeftWidth:
        return m_originalToValue.getValue() ==
               newStyle->border().left().width();
    case CSSStyleValuePair::KeyKind::PaddingTop:
        return m_originalToValue.getValue() == newStyle->padding().top();
    case CSSStyleValuePair::KeyKind::PaddingRight:
        return m_originalToValue.getValue() == newStyle->padding().right();
    case CSSStyleValuePair::KeyKind::PaddingBottom:
        return m_originalToValue.getValue() == newStyle->padding().bottom();
    case CSSStyleValuePair::KeyKind::PaddingLeft:
        return m_originalToValue.getValue() == newStyle->padding().left();
    case CSSStyleValuePair::KeyKind::Left:
        return m_originalToValue.getValue() == newStyle->left();
    case CSSStyleValuePair::KeyKind::Top:
        return m_originalToValue.getValue() == newStyle->top();
    case CSSStyleValuePair::KeyKind::Right:
        return m_originalToValue.getValue() == newStyle->right();
    case CSSStyleValuePair::KeyKind::Bottom:
        return m_originalToValue.getValue() == newStyle->bottom();
    case CSSStyleValuePair::KeyKind::BackgroundPositionX: {
        size_t layerIndex = this->layerIndex();
        return layerIndex < newStyle->backgroundLayerSize() &&
               m_originalToValue.getValue() ==
                   newStyle->backgroundPositionX(layerIndex);
    }
    case CSSStyleValuePair::KeyKind::BackgroundPositionY: {
        size_t layerIndex = this->layerIndex();
        return layerIndex < newStyle->backgroundLayerSize() &&
               m_originalToValue.getValue() ==
                   newStyle->backgroundPositionY(layerIndex);
    }
    case CSSStyleValuePair::KeyKind::FontSize:
        return m_originalToValue.getValue().fixed() ==
               newStyle->fixedFontSize();
    default:
        return false;
    }

    return false;
}

ActiveLengthSizeAnimationTask::ActiveLengthSizeAnimationTask(
    const ActiveAnimationTaskInit& init, Optional<LengthSize> originalToValue)
    : ActiveAnimationTask(init)
{
    // Note that this originated from legacy code.
    if (init.animationType == AnimationType::Transition) {
        m_originalToValue = originalToValue;
    } else {
        m_isEveryAnimiatedValueResolved = true;
    }
}

void* ActiveLengthSizeAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveLengthSizeAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ActiveLengthSizeAnimationTask)] = {
            0
        };
        fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(ActiveLengthSizeAnimationTask));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

static LengthSize interpolateLengthSize(double progress,
                                        AnimatedValue fromValue,
                                        AnimatedValue toValue, bool isForward)
{
#define INTERPOLATE_LENGTHSIZE(WH)                                     \
    Length(from->WH().type(),                                          \
           interpolate(from->WH().numberData(), to->WH().numberData(), \
                       progress, isForward))

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

void ActiveLengthSizeAnimationTask::execute(double progress,
                                            ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        style->setBackgroundSize(
            interpolateLengthSize(progress, *currentAnimatedFromValue(),
                                  *currentAnimatedToValue(), m_isForward),
            layerIndex());
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

bool ActiveLengthSizeAnimationTask::isKindOfTransitionProperty(
    CSSStyleValuePair::KeyKind key)
{
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        return key == CSSStyleValuePair::KeyKind::BackgroundSize ||
               key == CSSStyleValuePair::KeyKind::Background;
    }
    return key == m_property;
}

bool ActiveLengthSizeAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        size_t layerIndex = this->layerIndex();
        if (layerIndex < newStyle->backgroundLayerSize() &&
            newStyle->backgroundSizeIsLength(layerIndex) &&
            m_originalToValue ==
                newStyle->backgroundSizeLengthValue(layerIndex)) {
            return true;
        }
    }
    return false;
}

ActiveVisibilityAnimationTask::ActiveVisibilityAnimationTask(
    const ActiveAnimationTaskInit& init)
    : ActiveAnimationTask(init)
{
    if (init.animationType != AnimationType::Transition) {
        // Note that this originated from legacy code.
        m_isEveryAnimiatedValueResolved = true;
    }
}

void ActiveVisibilityAnimationTask::execute(double progress,
                                            ComputedStyle* style)
{
    if (currentAnimatedToValue()->getVisibilityValue() ==
        VisibilityValue::VisibleVisibilityValue) {
        if (progress) {
            style->setVisibility(
                currentAnimatedToValue()->getVisibilityValue());
        } else {
            style->setVisibility(
                currentAnimatedFromValue()->getVisibilityValue());
        }
    } else {
        if (progress == 1) {
            style->setVisibility(
                currentAnimatedToValue()->getVisibilityValue());
        } else {
            style->setVisibility(
                currentAnimatedFromValue()->getVisibilityValue());
        }
    }
}

bool ActiveVisibilityAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    if (m_property == CSSStyleValuePair::KeyKind::Visibility) {
        return newStyle->visibility() ==
               currentAnimatedToValue()->getVisibilityValue();
    }
    return false;
}

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* fromStyle, Optional<Frame*> oldFrame,
    ComputedStyle* toStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, double>>&
        canceledAnimationProgress)
{
    TransitionApplier transitionApplier(element, fromStyle, oldFrame, toStyle,
                                        damagedKeys);
    return transitionApplier.apply();
}

ActiveSVGLengthAnimationTask::ActiveSVGLengthAnimationTask(
    const ActiveAnimationTaskInit& init, AtomicString attributeName)
    : ActiveAnimationTask(init)
    , m_attributeName(attributeName)
{
    STARFISH_ASSERT(init.animationType == AnimationType::SVGAnimation);
    m_isEveryAnimiatedValueResolved = true;
}

void* ActiveSVGLengthAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveSVGLengthAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ActiveSVGLengthAnimationTask)] = {
            0
        };
        fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(ActiveSVGLengthAnimationTask));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ActiveSVGLengthAnimationTask::execute(double progress)
{
    auto a1 = currentAnimatedFromValue();
    auto a2 = currentAnimatedToValue();
    if (a1->isTransformData() && a2->isTransformData()) {
        Optional<StyleTransformData*> transformValue;
        auto t1 = a1->getTransformData()->at(0);
        auto t2 = a2->getTransformData()->at(0);
        STARFISH_ASSERT(t1.type() == t2.type());

        transformValue = new StyleTransformData(t1.clone());

        switch (t1.type()) {
        case StyleTransformData::Translate: {
            auto d1 = t1.translate();
            auto d2 = t2.translate();
            double tx =
                d1->tx().fixed() * (1 - progress) + d2->tx().fixed() * progress;
            double ty =
                d1->ty().fixed() * (1 - progress) + d2->ty().fixed() * progress;
            transformValue->translate()->setData(Length(Length::Fixed, tx),
                                                 Length(Length::Fixed, ty));
        } break;
        case StyleTransformData::Scale: {
            auto d1 = t1.scale();
            auto d2 = t2.scale();
            double tx = d1->x() * (1 - progress) + d2->x() * progress;
            double ty = d1->y() * (1 - progress) + d2->y() * progress;
            transformValue->scale()->setData(tx, ty);
        } break;
        case StyleTransformData::Rotate: {
            auto d1 = t1.rotate();
            auto d2 = t2.rotate();
            double newAngle =
                d1->angle() * (1 - progress) + d2->angle() * progress;
            double newCX = d1->cx().numberData() * (1 - progress) +
                           d2->cx().numberData() * progress;
            double newCY = d1->cy().numberData() * (1 - progress) +
                           d2->cy().numberData() * progress;
            transformValue->rotate()->setData(newAngle,
                                              Length(Length::Fixed, newCX),
                                              Length(Length::Fixed, newCY));
        } break;
        case StyleTransformData::Skew: {
            auto d1 = t1.skew();
            auto d2 = t2.skew();
            double x = d1->angleX() * (1 - progress) + d2->angleX() * progress;
            double y = d1->angleY() * (1 - progress) + d2->angleY() * progress;
            transformValue->skew()->setData(x, y);
        } break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
        }

        m_targetElement->asSVGElement()->setAnimatedAttribute(
            m_attributeName, NullOption, transformValue, this);
        return;
    } else if (a1->isAnimateMotion()) {
        auto pt = SVGAnimateMotionElement::computePoint(
            *a1->getAnimateMotionValue(), progress);
        // NOTE use dx, dy to store point
        // it is not orientated from spec
        m_targetElement->asSVGElement()->setAnimatedAttribute(
            m_targetElement->starfish()
                ->staticStrings()
                ->m_dx.localNameAtomic(),
            Length(Length::Fixed, pt.x()), NullOption, this);
        m_targetElement->asSVGElement()->setAnimatedAttribute(
            m_targetElement->starfish()
                ->staticStrings()
                ->m_dy.localNameAtomic(),
            Length(Length::Fixed, pt.y()), NullOption, this);
        m_targetElement->setNeedsLayout();
        return;
    }

    Length newLength =
        computeLengthAnimationValue(a1, a2, progress, m_isForward);
    m_targetElement->asSVGElement()->setAnimatedAttribute(
        m_attributeName, newLength, NullOption, this);
}

void ActiveSVGLengthAnimationTask::execute(double progress,
                                           ComputedStyle* style)
{
    // DO NOTTHING
    // ActiveSVGLengthAnimationTask tasks are executed on
    // SVGElement::styleForPresentationAttribute
}

bool ActiveSVGLengthAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    return true;
}

void ActiveSVGLengthAnimationTask::attachToElement()
{
    m_targetElement->asSVGElement()->setAnimatedAttribute(
        m_attributeName, NullOption, NullOption, this);
}

void ActiveSVGLengthAnimationTask::detachFromElement()
{
    m_targetElement->asSVGElement()->removeAnimatedAttribute(m_attributeName,
                                                             this);
}

bool ActiveSVGLengthAnimationTask::needsContinuousRendering(uint64_t tick)
{
    if (m_targetElement->asSVGElement()->isRenderableElement()) {
        if (!ActiveAnimationTask::needsContinuousRendering(tick)) {
            return false;
        }
    }
    auto remainTime = this->remainTime(tick);

    if (remainTime <= 0 && fillMode() == AnimationFillModeValue::None &&
        !isInForwardsFillMode()) {
        return false;
    }
    return true;
}

ActiveFilterAnimationTask::ActiveFilterAnimationTask(
    const ActiveAnimationTaskInit& init)
    : ActiveAnimationTask(init)
{
}

void ActiveFilterAnimationTask::execute(double progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    FilterFunction* from = currentAnimatedFromValue()->getFilterValue();
    FilterFunction* to = currentAnimatedToValue()->getFilterValue();

    STARFISH_ASSERT(from->type() == to->type());

    if (to->type() == FilterFunctionType::BlurFilterFunctionType) {
        BlurFilterFunction* interpolatedBlur = interpolateBlurFilter(
            static_cast<BlurFilterFunction*>(from),
            static_cast<BlurFilterFunction*>(to), progress);
        applyBlurFilterToStyle(interpolatedBlur, style);
    } else {
        STARFISH_UNIMPLEMENTED();
    }
}

BlurFilterFunction* ActiveFilterAnimationTask::interpolateBlurFilter(
    BlurFilterFunction* from, BlurFilterFunction* to, double progress)
{
    BlurFilterFunction* newBlurFunction =
        new BlurFilterFunction(*from->toCSSFilterFunction());

    float fromStandardDeviation = from->standardDeviation().fixed();
    float toStandardDeviation = to->standardDeviation().fixed();
    float interpolatedStandardDeviation = interpolate(
        fromStandardDeviation, toStandardDeviation, progress, m_isForward);

    newBlurFunction->setStandardDeviation(
        Length(Length::Fixed, interpolatedStandardDeviation));

    return newBlurFunction;
}

void ActiveFilterAnimationTask::applyBlurFilterToStyle(
    BlurFilterFunction* blurFilter, ComputedStyle* style)
{
    CSSStyleValuePair newValue;
    newValue.setKeyKind(CSSStyleValuePair::KeyKind::Filter);

    ValueList* list = new ValueList(Separator::SpaceSeparator);
    list->emplace_back(CSSStyleValuePair::ValueKind::FilterFunctionValueKind,
                       blurFilter->toCSSFilterFunction());
    newValue.setValueList(list);

    style->setFilter(FilterFunctions::create(newValue));
}

} // namespace Starfish
