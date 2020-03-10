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
#include "core/animation/AnimationUtil.h"
#include "core/animation/TimingFunction.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/AnimationEvent.h"
#include "core/dom/TransitionEvent.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/StackingContext.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/style/CalcData.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSProperty.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "platform/window/PlatformWindow.h"

namespace Starfish {

template <typename T>
static float interpolate(const T from, const T to, float progress,
                         bool isForward = true)
{
    if (isForward == true) {
        return from + (to - from) * progress;
    } else {
        return from + (to - from) * (1 - progress);
    }
}

void AnimationExecutor::checkActiveExecutorInWebView()
{
    auto& v = window()->webView()->m_activeAnimationExecutor;

    if (m_activeTransitions.size() > 0 || m_activeAnimations->size() > 0) {
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

void AnimationExecutor::fireAnimationStartEvent(Element* element, String* name,
                                                double delay)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(name != nullptr);
    // STARFISH_LOG_INFO("element %p animationStart: animationName [%s]\n",
    // element, name->toUTF8NonGCString().data());
    AnimationEventInit init;
    init.setAnimationName(name);
    if (delay < 0) {
        init.setElapsedTime(-(delay / 1000));
    } else {
        init.setElapsedTime(0);
    }
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    AnimationEvent* event = new AnimationEvent(
        element->executionContext(),
        element->starfish()->staticStrings()->m_animationstart.localName(),
        init);
    element->dispatchEventIdleTimeByUA(event);
}

void AnimationExecutor::fireAnimationEndEvent(Element* element, String* name,
                                              float elapsedTime)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(name != nullptr);
    // STARFISH_LOG_INFO("element %p animationEnd: animationName [%s]\n",
    // element, name->toUTF8NonGCString().data());
    AnimationEventInit init;
    init.setAnimationName(name);
    init.setElapsedTime(elapsedTime);
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    AnimationEvent* event = new AnimationEvent(
        element->executionContext(),
        element->starfish()->staticStrings()->m_animationend.localName(), init);
    element->dispatchEventIdleTimeByUA(event);
}

void AnimationExecutor::fireAnimationCancelEvent(Element* element, String* name,
                                                 float elapsedTime)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(name != nullptr);
    // STARFISH_LOG_INFO("element %p animationCancel: animationName [%s]\n",
    // element, name->toUTF8NonGCString().data());
    AnimationEventInit init;
    init.setAnimationName(name);
    init.setElapsedTime(elapsedTime);
    init.setBubbles(true);
    init.setCancelable(false);
    // TODO add more information to init
    AnimationEvent* event = new AnimationEvent(
        element->executionContext(),
        element->starfish()->staticStrings()->m_animationcancel.localName(),
        init);
    element->dispatchEventIdleTimeByUA(event);
}

ActiveAnimationTask::ActiveAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const AnimatedValue& from, const AnimatedValue& to, uint64_t durationInms,
    uint64_t delayInms, TimingFunction* timingFunction)
    : m_isEveryAnimiatedValueResolved(true)
    , m_type(TRANSITION_TYPE)
    , m_property(targetProperty)
    , m_targetElement(target)
    , m_startTimeMs(0)
    , m_durationMs(durationInms)
    , m_startDelayMs(delayInms)
    , m_delayMs(delayInms)
    , m_playState(AnimationPlayStateValue::AnimationPlayStateRunningValue)
    , m_fillMode(AnimationFillModeValue::AnimationFillModeNoneValue)
    , m_gapTimeMs(0)
    , m_iterationCount(0)
    , m_iterationStart(0)
    , m_isInDelayedTime(delayInms > 0 ? true : false)
    , m_isForward(true)
    , m_isRunning(true)
    , m_isCSSAnimationTask(true)
    , m_isInForwardsFillMode(false)
    , m_frameIdx(0)
    , m_frameSize(2)
{
    STARFISH_ASSERT(target != nullptr);
    STARFISH_ASSERT(timingFunction != nullptr);

    m_values.push_back(new AnimatedValue(from));
    m_values.push_back(new AnimatedValue(to));
    m_timingFunctions.push_back(timingFunction);
}

ActiveAnimationTask::ActiveAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const GCVector<AnimatedValue*>& values,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
    uint64_t delayInms, float iterationCount, AnimationPlayStateValue playState,
    AnimationFillModeValue fillMode)
    : m_isEveryAnimiatedValueResolved(false)
    , m_type(ANIMATION_TYPE)
    , m_property(targetProperty)
    , m_targetElement(target)
    , m_startTimeMs(0)
    , m_durationMs(durationInms)
    , m_startDelayMs(delayInms)
    , m_delayMs(delayInms)
    , m_playState(playState)
    , m_fillMode(fillMode)
    , m_gapTimeMs(0)
    , m_iterationCount(iterationCount)
    , m_iterationStart(0)
    , m_isInDelayedTime(delayInms > 0 ? true : false)
    , m_isForward(true)
    , m_isRunning(true)
    , m_isCSSAnimationTask(true)
    , m_isInForwardsFillMode(false)
    , m_frameIdx(0)
    , m_frameSize(values.size())
{
    STARFISH_ASSERT(target != nullptr);
    STARFISH_ASSERT(values.size() > 1);
    STARFISH_ASSERT(offsets.size() > 1);
    STARFISH_ASSERT(timingFunctions.size() > 1);

    m_values.assign(values.begin(), values.end());
    m_offsets.assign(offsets.begin(), offsets.end());
    m_timingFunctions.assign(timingFunctions.begin(), timingFunctions.end());
}

void* ActiveAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ActiveAnimationTask)] = { 0 };
        fillGCDescriptor(obj_bitmap);
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ActiveAnimationTask));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void ActiveAnimationTask::step(uint64_t currentTickCount, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    float f = 0;
    if (m_startTimeMs != 0) {
        f = fraction(currentTickCount);
    }

    if (m_type == ANIMATION_TYPE) {
        if ((m_isInDelayedTime == true && f == 0) ||
            m_isEveryAnimiatedValueResolved == false) {
            return;
        }

        execute(computeProgress(f), style);

        if (!std::isinf(m_iterationCount) && m_gapTimeMs == 0 &&
            m_fillMode ==
                AnimationFillModeValue::AnimationFillModeForwardsValue) {
            return;
        }

        if (f >= 1.0 && m_isForward == true) {
            m_frameIdx++;
            if (m_frameIdx == m_frameSize - 1) {
                m_frameIdx = 0;
                m_startTimeMs = 0;
                m_delayMs = 0;
                m_isInDelayedTime = false;
            }
        } else if (f <= 0.0 && m_isForward == false) {
            m_frameIdx--;
            if (m_frameIdx == 0) {
                m_frameIdx = m_frameSize - 1;
                m_startTimeMs = 0;
                m_delayMs = 0;
                m_isInDelayedTime = false;
            }
        }

    } else {
        execute(computeProgress(f), style);
    }
}

void ActiveAnimationTask::fireTransitionStartEvent()
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

void ActiveAnimationTask::fireTransitionEndEvent()
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

void ActiveAnimationTask::fireTransitionCancelEvent()
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

AnimatedValue* ActiveAnimationTask::currentAnimatedFromValue()
{
    return m_values[m_frameIdx];
}

AnimatedValue* ActiveAnimationTask::currentAnimatedToValue()
{
    if (m_isForward == true) {
        return m_values[m_frameIdx + 1];
    } else {
        return m_values[m_frameIdx - 1];
    }
}

TimingFunction* ActiveAnimationTask::currentTimingFunction()
{
    return m_timingFunctions[m_frameIdx];
}

float ActiveAnimationTask::computeProgress(float& fraction)
{
    STARFISH_ASSERT(fraction >= 0.0f);
    STARFISH_ASSERT(fraction <= 1.0f);

    if (m_type == ANIMATION_TYPE) {
        if (m_isForward == true) {
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
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const GCVector<AnimatedValue*>& values,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
    uint64_t delayInms, float iterationCount, AnimationPlayStateValue playState,
    AnimationFillModeValue fillMode)
    : ActiveAnimationTask(target, targetProperty, values, offsets,
                          timingFunctions, durationInms, delayInms,
                          iterationCount, playState, fillMode)
{
    m_isEveryAnimiatedValueResolved = true;
    STARFISH_ASSERT(target != nullptr);
}

void ActiveOpacityAnimationTask::execute(float progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    float from = currentAnimatedFromValue()->getFloat();
    float to = currentAnimatedToValue()->getFloat();
    float value = 0;
    if (m_isForward == true) {
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
    uint64_t delayInms, TimingFunction* timingFunction,
    StyleTransformDataGroup* orgTransformValue)
    : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                          delayInms, timingFunction)
    , m_originalTransformValue(nullptr)
{
    STARFISH_ASSERT(target != nullptr);
    STARFISH_ASSERT(timingFunction != nullptr);
    STARFISH_ASSERT(m_values.size() == 2);

    if (orgTransformValue != nullptr) {
        StyleTransformDataGroup* newOrgData = new StyleTransformDataGroup();
        for (size_t i = 0; i < orgTransformValue->size(); i++) {
            newOrgData->append(orgTransformValue->at(i));
        }
        m_originalTransformValue = newOrgData;
    }

    m_decomposedFrom = decomposing2DMatrix(m_values[0]->getMatrix());
    m_decomposedTo = decomposing2DMatrix(m_values[1]->getMatrix());

    matrixInterpolationPreprocessing(m_decomposedFrom, m_decomposedTo);
}

ActiveTransformAnimationTask::ActiveTransformAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const GCVector<AnimatedValue*>& values,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
    uint64_t delayInms, float iterationCount, AnimationPlayStateValue playState,
    AnimationFillModeValue fillMode)
    : ActiveAnimationTask(target, targetProperty, values, offsets,
                          timingFunctions, durationInms, delayInms,
                          iterationCount, playState, fillMode)
    , m_originalTransformValue(nullptr)
{
    m_isEveryAnimiatedValueResolved = false;
    STARFISH_ASSERT(target != nullptr);
}

void ActiveTransformAnimationTask::didAnimationFrameChanged()
{
    ActiveAnimationTask::didAnimationFrameChanged();

    m_decomposedFrom =
        decomposing2DMatrix(currentAnimatedFromValue()->getMatrix());
    m_decomposedTo = decomposing2DMatrix(currentAnimatedToValue()->getMatrix());

    matrixInterpolationPreprocessing(m_decomposedFrom, m_decomposedTo);
}

void ActiveTransformAnimationTask::resolveUnresolvedAnimatedValues()
{
    if (m_isEveryAnimiatedValueResolved == false) {
        Frame* frm = m_targetElement->frame();

        if (frm->isTransformable() == true) {
            for (size_t i = 0; i < m_values.size(); i++) {
                STARFISH_ASSERT(m_values[i]->isTransformData() == true);
                new (m_values[i])
                    AnimatedValue(ComputedStyle::transformToMatrix(
                        m_values[i]->getTransformData(),
                        frm->asFrameBox()->width(), frm->asFrameBox()->height(),
                        frm));
            }
        } else {
            for (size_t i = 0; i < m_values.size(); i++) {
                STARFISH_ASSERT(m_values[i]->isTransformData() == true);
                new (m_values[i]) AnimatedValue(SkMatrix::I());
            }
        }

        if (m_isForward == true) {
            m_decomposedFrom = decomposing2DMatrix(m_values[0]->getMatrix());
            m_decomposedTo = decomposing2DMatrix(m_values[1]->getMatrix());
        } else {
            m_decomposedFrom =
                decomposing2DMatrix(m_values[m_values.size() - 1]->getMatrix());
            m_decomposedTo =
                decomposing2DMatrix(m_values[m_values.size() - 2]->getMatrix());
        }

        matrixInterpolationPreprocessing(m_decomposedFrom, m_decomposedTo);
    }

    ActiveAnimationTask::resolveUnresolvedAnimatedValues();
}

void* ActiveTransformAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveTransformAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
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

void ActiveTransformAnimationTask::execute(float progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    Element* current = targetElement();
    auto transforms = style->rareComputedStyleData()->transforms();

    didAnimationFrameChanged();

    MatrixDecomposed2D now;
    if (m_isForward == true) {
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
    } else {
        now.angle = m_decomposedFrom.angle * progress +
                    m_decomposedTo.angle * (1 - progress);
        now.matrixM11 = m_decomposedFrom.matrixM11 * progress +
                        m_decomposedTo.matrixM11 * (1 - progress);
        now.matrixM12 = m_decomposedFrom.matrixM12 * progress +
                        m_decomposedTo.matrixM12 * (1 - progress);
        now.matrixM21 = m_decomposedFrom.matrixM21 * progress +
                        m_decomposedTo.matrixM21 * (1 - progress);
        now.matrixM22 = m_decomposedFrom.matrixM22 * progress +
                        m_decomposedTo.matrixM22 * (1 - progress);
        now.scaleX = m_decomposedFrom.scaleX * progress +
                     m_decomposedTo.scaleX * (1 - progress);
        now.scaleY = m_decomposedFrom.scaleY * progress +
                     m_decomposedTo.scaleY * (1 - progress);
        now.translateX = m_decomposedFrom.translateX * progress +
                         m_decomposedTo.translateX * (1 - progress);
        now.translateY = m_decomposedFrom.translateY * progress +
                         m_decomposedTo.translateY * (1 - progress);
    }
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
    STARFISH_ASSERT(newStyle != nullptr);
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
    STARFISH_ASSERT(style != nullptr);

    Unit::Color from = currentAnimatedFromValue()->getColor();
    Unit::Color to = currentAnimatedToValue()->getColor();

    unsigned char r, g, b, a;
    if (m_isForward == true) {
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
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

bool ActiveColorAnimationTask::taskCanContinue(ComputedStyle* newStyle)
{
    STARFISH_ASSERT(newStyle != nullptr);
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        if (newStyle->backgroundColor() ==
            currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottomColor) {
        if (newStyle->border().bottom().color() ==
            currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeftColor) {
        if (newStyle->border().left().color() ==
            currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRightColor) {
        if (newStyle->border().right().color() ==
            currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTopColor) {
        if (newStyle->border().top().color() ==
            currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::Color) {
        if (newStyle->color() == currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::CaretColor) {
        if (newStyle->caretColor() == currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::OutlineColor) {
        if (newStyle->outlineColor() == currentAnimatedToValue()->getColor()) {
            return true;
        }
    } else if (m_property == CSSStyleValuePair::KeyKind::TextDecorationColor) {
        if (newStyle->textDecorationColor() ==
            currentAnimatedToValue()->getColor()) {
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
    uint64_t delayInms, TimingFunction* timingFunction, Length originalToValue,
    size_t indexForBgLayer)
    : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                          delayInms, timingFunction)
    , m_originalToValue(originalToValue)
    , m_indexForBgLayer(indexForBgLayer)
{
    STARFISH_ASSERT(target != nullptr);
    STARFISH_ASSERT(timingFunction != nullptr);
}

ActiveLengthAnimationTask::ActiveLengthAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const GCVector<AnimatedValue*>& values,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
    uint64_t delayInms, float iterationCount, AnimationPlayStateValue playState,
    AnimationFillModeValue fillMode, size_t indexForBgLayer)
    : ActiveAnimationTask(target, targetProperty, values, offsets,
                          timingFunctions, durationInms, delayInms,
                          iterationCount, playState, fillMode)
    , m_indexForBgLayer(indexForBgLayer)
{
    m_isEveryAnimiatedValueResolved = true;
    STARFISH_ASSERT(target != nullptr);
}

void* ActiveLengthAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveLengthAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
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
    if (m_isEveryAnimiatedValueResolved == false) {
        Frame* frm = m_targetElement->frame();
        FrameBox* cb = containingBlock(frm);

        LayoutUnit parentLength;
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
                    m_targetElement->style(), m_indexForBgLayer, posSize,
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
                    m_targetElement->style(), m_indexForBgLayer, posSize,
                    imgSize);
                if (imgSize.width() == 0.0 || imgSize.height() == 0.0) {
                    return;
                }
                parentLength = posSize.height() - imgSize.height();
                break;
            }
            case CSSStyleValuePair::FontSize:
                break;
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
            STARFISH_ASSERT(m_values[i]->isLength() == true);
            if (m_values[i]->getLength().isAuto() == false) {
                STARFISH_ASSERT(m_values[i]->getLength().isDefinite(true) ==
                                true);
                if (m_values[i]->getLength().isDefinite(
                        isValueKindDependsOnParentFixedHeight == false ||
                        hasParentHeightFixedHeight == true) == true) {
                    new (m_values[i]) AnimatedValue(Length(
                        Length::Fixed, m_values[i]->getLength().specifiedValue(
                                           parentLength, m_targetElement)));
                } else {
                    new (m_values[i]) AnimatedValue();
                }
            }
        }
    }

    ActiveAnimationTask::resolveUnresolvedAnimatedValues();
}

void ActiveLengthAnimationTask::execute(float progress, ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    Length newLength;

    if (m_isEveryAnimiatedValueResolved == false) {
        newLength = currentAnimatedFromValue()->getLength();
    } else {
        AnimatedValue* fromValue = currentAnimatedFromValue();
        AnimatedValue* toValue = currentAnimatedToValue();

        if (fromValue->isLength() == false || toValue->isLength() == false) {
            // newLength remains as auto
        } else if (fromValue->getLength().isAuto() == true ||
                   toValue->getLength().isAuto() == true) {
            if (progress < 0.5f) {
                if (fromValue->isLength() == true) {
                    newLength = fromValue->getLength();
                } else {
                    newLength = Length();
                }
            } else {
                if (toValue->isLength() == true) {
                    newLength = toValue->getLength();
                } else {
                    newLength = Length();
                }
            }
        } else {
            if (toValue->getLength().isPercent() == true) {
                float fromPercent = fromValue->getLength().percent();
                float toPercent = toValue->getLength().percent();
                newLength =
                    Length(Length::Percent, interpolate(fromPercent, toPercent,
                                                        progress, m_isForward));
            } else {
                float fromFixed = fromValue->getLength().fixed();
                float toFixed = toValue->getLength().fixed();
                newLength =
                    Length(Length::Fixed, interpolate(fromFixed, toFixed,
                                                      progress, m_isForward));
            }
        }
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
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderTopWidth) {
        style->setBorderTopWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderRightWidth) {
        style->setBorderRightWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderBottomWidth) {
        style->setBorderBottomWidth(newLength);
    } else if (m_property == CSSStyleValuePair::KeyKind::BorderLeftWidth) {
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
    STARFISH_ASSERT(newStyle != nullptr);
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
    uint64_t delayInms, TimingFunction* timingFunction,
    LengthSize originalToValue, size_t indexForBgLayer)
    : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                          delayInms, timingFunction)
    , m_originalToValue(originalToValue)
    , m_indexForBgLayer(indexForBgLayer)
{
    STARFISH_ASSERT(target != nullptr);
    STARFISH_ASSERT(timingFunction != nullptr);
}

ActiveLengthSizeAnimationTask::ActiveLengthSizeAnimationTask(
    Element* target, CSSStyleValuePair::KeyKind targetProperty,
    const GCVector<AnimatedValue*>& values,
    const GCAtomicVector<double>& offsets,
    const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
    uint64_t delayInms, float iterationCount, AnimationPlayStateValue playState,
    AnimationFillModeValue fillMode, size_t indexForBgLayer)
    : ActiveAnimationTask(target, targetProperty, values, offsets,
                          timingFunctions, durationInms, delayInms,
                          iterationCount, playState, fillMode)
    , m_indexForBgLayer(indexForBgLayer)
{
    m_isEveryAnimiatedValueResolved = true;
    STARFISH_ASSERT(target != nullptr);
}

void* ActiveLengthSizeAnimationTask::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ActiveLengthSizeAnimationTask));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
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

static LengthSize interpolateLengthSize(float progress, AnimatedValue fromValue,
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

void ActiveLengthSizeAnimationTask::execute(float progress,
                                            ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundSize) {
        style->setBackgroundSize(
            interpolateLengthSize(progress, *currentAnimatedFromValue(),
                                  *currentAnimatedToValue(), m_isForward),
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
    STARFISH_ASSERT(newStyle != nullptr);
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

#define STARFISH_ASSERT_INPUT_LENGTH_FIXED()                             \
    STARFISH_ASSERT(currentAnimatedFromValue()->isLength() &&            \
                    currentAnimatedToValue()->isLength());               \
    STARFISH_ASSERT(currentAnimatedFromValue()->getLength().isFixed() && \
                    currentAnimatedToValue()->getLength().isFixed());

#define STARFISH_ASSERT_INPUT_LENGTH_FIXED_OR_PERCENT()                     \
    STARFISH_ASSERT(currentAnimatedFromValue()->isLength() &&               \
                    currentAnimatedToValue()->isLength());                  \
    STARFISH_ASSERT((currentAnimatedFromValue()->getLength().isFixed() &&   \
                     currentAnimatedToValue()->getLength().isFixed()) ||    \
                    (currentAnimatedFromValue()->getLength().isPercent() && \
                     currentAnimatedToValue()->getLength().isPercent()));

#define _DAMAGED_KEYS(PropName, ...) (damagedKeys[PropName])
#define NEED_TRANSITION(...)       \
    (_DAMAGED_KEYS(__VA_ARGS__) && \
     (isPropertyAll || _checkCSSProperty(property, __VA_ARGS__)))
#define CHECK_ANIMATION(...) (_checkCSSProperty(keyKind, __VA_ARGS__))

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* oldStyle, Frame* oldFrame,
    ComputedStyle* newStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, float>>&
        canceledAnimationProgress)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(oldStyle != nullptr);
    STARFISH_ASSERT(oldFrame != nullptr);
    STARFISH_ASSERT(newStyle != nullptr);
    STARFISH_ASSERT(damagedKeys != nullptr);
    bool ret = false;

    StyleTransitionData* data = newStyle->transition();
    AnimationExecutor* executor = element->document()->animationExecutor();

    for (size_t i = 0; i < data->size(); i++) {
        if (data->duration(i).toTimeValue() == 0) {
            continue;
        }

        bool gotTransition = false;
        CSSStyleValuePair::KeyKind property = data->property(i);
        bool isPropertyAll = property == CSSStyleValuePair::All;

        auto duration = data->duration(i).toTimeValue();
        auto delay = data->delay(i).toTimeValue();
        auto timingFunction = data->timingFunction(i);

        if (NEED_TRANSITION(CSSStyleValuePair::Opacity) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::Opacity);
            if (found == false) {
                auto task = new ActiveOpacityAnimationTask(
                    element, CSSStyleValuePair::Opacity,
                    AnimatedValue(oldStyle->opacity()),
                    AnimatedValue(newStyle->opacity()), duration, delay,
                    timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::Transform) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::Transform);
            if (found == false && oldFrame->isTransformable() == true) {
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
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        // color series
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundColor,
                            CSSStyleValuePair::Background) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BackgroundColor);
            if (found == false) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BackgroundColor,
                    AnimatedValue(oldStyle->backgroundColor()),
                    AnimatedValue(newStyle->backgroundColor()), duration, delay,
                    timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderBottomColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderBottom) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderBottomColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->border().bottom().color();
                Unit::Color newColor = newStyle->border().bottom().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderBottomColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderLeftColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderLeft) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderLeftColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->border().left().color();
                Unit::Color newColor = newStyle->border().left().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderLeftColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderRightColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderRight) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderRightColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->border().right().color();
                Unit::Color newColor = newStyle->border().right().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderRightColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderTopColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderTop) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderTopColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->border().top().color();
                Unit::Color newColor = newStyle->border().top().color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderTopColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Color) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::Color);
            if (found == false) {
                Unit::Color oldColor = oldStyle->color();
                Unit::Color newColor = newStyle->color();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::Color, AnimatedValue(oldColor),
                    AnimatedValue(newColor), duration, delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::CaretColor) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::CaretColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->caretColor();
                Unit::Color newColor = newStyle->caretColor();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::CaretColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::OutlineColor) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::OutlineColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->outlineColor();
                Unit::Color newColor = newStyle->outlineColor();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::OutlineColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::TextDecorationColor,
                            CSSStyleValuePair::TextDecoration) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::TextDecorationColor);
            if (found == false) {
                Unit::Color oldColor = oldStyle->textDecorationColor();
                Unit::Color newColor = newStyle->textDecorationColor();
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::TextDecorationColor,
                    AnimatedValue(oldColor), AnimatedValue(newColor), duration,
                    delay, timingFunction);
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }
        // <- color series

        // length series
        if (NEED_TRANSITION(CSSStyleValuePair::Width) == true &&
            oldFrame != nullptr && oldFrame->isFrameBox() == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::Width);
            if (found == false) {
                auto oldWidth = oldStyle->width();
                auto newWidth = newStyle->width();

                if ((oldWidth.isPercent() == true ||
                     oldWidth.isFixed() == true) &&
                    (newWidth.isPercent() == true ||
                     newWidth.isFixed() == true)) {
                    auto fromValue = oldStyle->width();
                    auto toValue = newStyle->width();

                    if (toValue.isPercent() == true &&
                        !fromValue.isPercent() == true) {
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
                    } else if (toValue.isFixed() == true &&
                               fromValue.isFixed() == false) {
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
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::Height) == true &&
            oldFrame != nullptr && oldFrame->isFrameBox() == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::Height);
            if (found == false) {
                auto oldHeight = oldStyle->height();
                auto newHeight = newStyle->height();

                if ((oldHeight.isPercent() == true ||
                     oldHeight.isFixed() == true) &&
                    (newHeight.isPercent() == true ||
                     newHeight.isFixed() == true)) {
                    auto fromValue = oldStyle->height();
                    auto toValue = newStyle->height();

                    if (toValue.isPercent() == true &&
                        fromValue.isPercent() == false) {
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
                    } else if (toValue.isFixed() == true &&
                               fromValue.isFixed() == false) {
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
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MinWidth) == true &&
            oldFrame != nullptr && oldFrame->isFrameBox() == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MinWidth);
            if (found == false) {
                auto oldWidth = oldStyle->minWidth();
                auto newWidth = newStyle->minWidth();

                if ((oldWidth.isPercent() == true ||
                     oldWidth.isFixed() == true) &&
                    (newWidth.isPercent() == true ||
                     newWidth.isFixed() == true)) {
                    auto fromValue = oldStyle->minWidth();
                    auto toValue = newStyle->minWidth();

                    if (toValue.isPercent() == true &&
                        fromValue.isPercent() == false) {
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
                    } else if (toValue.isFixed() == true &&
                               fromValue.isFixed() == false) {
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
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MinHeight) == true &&
            oldFrame != nullptr && oldFrame->isFrameBox() == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MinHeight);
            if (found == false) {
                auto oldHeight = oldStyle->minHeight();
                auto newHeight = newStyle->minHeight();

                if ((oldHeight.isPercent() == true ||
                     oldHeight.isFixed() == true) &&
                    (newHeight.isPercent() == true ||
                     newHeight.isFixed() == true)) {
                    auto fromValue = oldStyle->minHeight();
                    auto toValue = newStyle->minHeight();

                    if (toValue.isPercent() == true &&
                        fromValue.isPercent() == false) {
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
                    } else if (toValue.isFixed() == true &&
                               fromValue.isFixed() == false) {
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
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MaxWidth) == true &&
            oldFrame != nullptr && oldFrame->isFrameBox() == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MaxWidth);
            if (found == false) {
                auto oldWidth = oldStyle->maxWidth();
                auto newWidth = newStyle->maxWidth();

                if ((oldWidth.isPercent() == true ||
                     oldWidth.isFixed() == true) &&
                    (newWidth.isPercent() == true ||
                     newWidth.isFixed() == true)) {
                    auto fromValue = oldStyle->maxWidth();
                    auto toValue = newStyle->maxWidth();

                    if (toValue.isPercent() == true &&
                        fromValue.isPercent() == false) {
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
                    } else if (toValue.isFixed() == true &&
                               fromValue.isFixed() == false) {
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
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MaxHeight) == true &&
            oldFrame != nullptr && oldFrame->isFrameBox() == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MaxHeight);
            if (found == false) {
                auto oldHeight = oldStyle->maxHeight();
                auto newHeight = newStyle->maxHeight();

                if ((oldHeight.isPercent() == true ||
                     oldHeight.isFixed() == true) &&
                    (newHeight.isPercent() == true ||
                     newHeight.isFixed() == true)) {
                    auto fromValue = oldStyle->maxHeight();
                    auto toValue = newStyle->maxHeight();

                    if (toValue.isPercent() == true &&
                        fromValue.isPercent() == false) {
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
                    } else if (toValue.isFixed() == true &&
                               fromValue.isFixed() == false) {
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
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginTop,
                            CSSStyleValuePair::Margin) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MarginTop);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::marginTopToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginTop, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().top());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginRight,
                            CSSStyleValuePair::Margin) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MarginRight);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::marginRightToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginRight, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().right());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginBottom,
                            CSSStyleValuePair::Margin) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MarginBottom);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::marginBottomToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginBottom, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().bottom());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::MarginLeft,
                            CSSStyleValuePair::Margin) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::MarginLeft);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::marginLeftToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginLeft, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->margin().left());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderTop,
                            CSSStyleValuePair::Border) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderTop);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::borderTopToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderTop, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().top().width());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderRight,
                            CSSStyleValuePair::Border) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderRight);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::borderRightToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderRight, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().right().width());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderBottom,
                            CSSStyleValuePair::Border) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderBottom);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::borderBottomToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderBottom, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().bottom().width());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::BorderLeft,
                            CSSStyleValuePair::Border) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::BorderLeft);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::borderLeftToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderLeft, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->border().left().width());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingTop,
                            CSSStyleValuePair::Padding) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::PaddingTop);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::paddingTopToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingTop, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->padding().top());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingRight,
                            CSSStyleValuePair::Padding) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::PaddingRight);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::paddingRightToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingRight, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->padding().right());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingBottom,
                            CSSStyleValuePair::Padding) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::PaddingBottom);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::paddingBottomToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingBottom,
                    AnimatedValue(v1), AnimatedValue(v2), duration, delay,
                    timingFunction, newStyle->padding().bottom());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::PaddingLeft,
                            CSSStyleValuePair::Padding) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::PaddingLeft);
            AnimatedValue v1, v2;
            if (found == false &&
                AnimationUtil::paddingLeftToAnimatedValue(
                    oldStyle, newStyle, element, v1, v2) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingLeft, AnimatedValue(v1),
                    AnimatedValue(v2), duration, delay, timingFunction,
                    newStyle->padding().left());
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

#define GEN_SIDE(Side, side)                                                 \
    if (NEED_TRANSITION(CSSStyleValuePair::Side) == true) {                  \
        bool found =                                                         \
            executor->hasActiveTransition(element, CSSStyleValuePair::Side); \
        AnimatedValue v1, v2;                                                \
        if (found == false &&                                                \
            AnimationUtil::lengthToAnimatedValue(oldStyle->side(),           \
                                                 newStyle->side(), element,  \
                                                 v1, v2) == true) {          \
            auto task = new ActiveLengthAnimationTask(                       \
                element, CSSStyleValuePair::Side, AnimatedValue(v1),         \
                AnimatedValue(v2), duration, delay, timingFunction,          \
                newStyle->side());                                           \
            executor->registerTransition(task, newStyle);                    \
            gotTransition = true;                                            \
        }                                                                    \
    }
        GEN_SIDE(Left, left)
        GEN_SIDE(Right, right)
        GEN_SIDE(Top, top)
        GEN_SIDE(Bottom, bottom)
#undef GEN_SIDE

        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionX,
                            CSSStyleValuePair::BackgroundPosition,
                            CSSStyleValuePair::Background) == true) {
            if (oldStyle->hasBlockLikeDisplay() == false ||
                newStyle->hasBlockLikeDisplay() == false) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            for (size_t i = 0; i < layerSize; i++) {
                bool found = executor->hasActiveTransition(
                    element, CSSStyleValuePair::BackgroundPositionX);
                AnimatedValue pos1, pos2;
                if (found == false &&
                    AnimationUtil::backgroundPosXToAnimatedValue(
                        oldStyle, newStyle, oldPaintingBox, element, pos1, pos2,
                        i) == true) {
                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionX,
                        AnimatedValue(pos1), AnimatedValue(pos2), duration,
                        delay, timingFunction, newStyle->backgroundPositionX(i),
                        i);
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionY,
                            CSSStyleValuePair::BackgroundPosition,
                            CSSStyleValuePair::Background) == true) {
            if (oldStyle->hasBlockLikeDisplay() == false ||
                newStyle->hasBlockLikeDisplay() == false) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            for (size_t i = 0; i < layerSize; i++) {
                bool found = executor->hasActiveTransition(
                    element, CSSStyleValuePair::BackgroundPositionY);
                AnimatedValue pos1, pos2;
                if (found == false &&
                    AnimationUtil::backgroundPosYToAnimatedValue(
                        oldStyle, newStyle, oldPaintingBox, element, pos1, pos2,
                        i) == true) {
                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionY,
                        AnimatedValue(pos1), AnimatedValue(pos2), duration,
                        delay, timingFunction, newStyle->backgroundPositionY(i),
                        i);
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        // NOTE background-size should come after background-position
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundSize,
                            CSSStyleValuePair::Background) == true) {
            if (oldStyle->hasBlockLikeDisplay() == false ||
                newStyle->hasBlockLikeDisplay() == false) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            for (size_t i = 0; i < layerSize; i++) {
                AnimatedValue size1, size2;
                bool found = executor->hasActiveTransition(
                    element, CSSStyleValuePair::BackgroundSize);
                if (found == false &&
                    AnimationUtil::backgroundSizeToAnimatedValue(
                        oldStyle, newStyle, oldPaintingBox, element, size1,
                        size2, i) == true) {
                    auto task = new ActiveLengthSizeAnimationTask(
                        element, CSSStyleValuePair::BackgroundSize,
                        AnimatedValue(size1), AnimatedValue(size2), duration,
                        delay, timingFunction,
                        newStyle->backgroundSizeLengthValue(i), i);
                    executor->registerTransition(task, newStyle);
                    gotTransition = true;
                }
            }
        }

        if (NEED_TRANSITION(CSSStyleValuePair::FontSize,
                            CSSStyleValuePair::Font) == true) {
            bool found = executor->hasActiveTransition(
                element, CSSStyleValuePair::FontSize);
            if (found == false) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::FontSize,
                    AnimatedValue(
                        Length(Length::Fixed, oldStyle->fixedFontSize())),
                    AnimatedValue(
                        Length(Length::Fixed, newStyle->fixedFontSize())),
                    duration, delay, timingFunction,
                    Length(Length::Fixed, newStyle->fixedFontSize()));
                executor->registerTransition(task, newStyle);
                gotTransition = true;
            }
        }

        // <- length series

        if (gotTransition == true) {
            // TODO reduce animation duration here with
            // canceledAnimationProgress
            ret = true;
        }
    }

    return ret;
}

static Length backgroundPositionToLength(const CSSStyleValuePair& property)
{
    Length value;
    if (property.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        SideValue side = property.sideValue();
        if (side == SideValue::LeftSideValue) {
            value = Length(Length::Percent, 0.0f);
        } else if (side == SideValue::RightSideValue) {
            value = Length(Length::Percent, 1.0f);
        } else if (side == SideValue::TopSideValue) {
            value = Length(Length::Percent, 0.0f);
        } else if (side == SideValue::BottomSideValue) {
            value = Length(Length::Percent, 1.0f);
        } else if (side == SideValue::CenterSideValue) {
            value = Length(Length::Percent, 0.5f);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (property.valueKind() == CSSStyleValuePair::ValueKind::Length) {
        value = property.lengthValue();
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::CalcValueKind) {
        value = property.lengthValue();
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::Percentage) {
        value = Length(Length::Percent, property.percentageValue());
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return value;
}

static AnimatedValue* backgroundPositionToAnimatedValue(
    ComputedStyle* style, Element* element, const CSSStyleValuePair& property,
    size_t layer)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() > layer) {
            return new AnimatedValue(
                backgroundPositionToLength((*list)[layer]));
        } else {
            return new AnimatedValue(
                backgroundPositionToLength((*list)[list->size() - 1]));
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return nullptr;
}

static Nullable<Length> convertValueToLength(const CSSStyleValuePair& property)
{
    CSSStyleValuePair::ValueKind kind = property.valueKind();
    CSSStyleValuePair::ValueData data = property.value();
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Length();
    } else if (kind == CSSStyleValuePair::ValueKind::Length) {
        return data.m_length.toLength();
    } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
        return Length(Length::Percent, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Length(Length::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->type();
        if (type.isLength() || type.isPercentage() || type.isNumber()) {
            return Length(data.m_calc);
        } else {
            return Nullable<Length>();
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return Nullable<Length>();
}

static LengthSize backgroundSizeToLengthSize(const CSSStyleValuePair& property)
{
    LengthSize result;
    if (property.valueKind() == CSSStyleValuePair::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() >= 1) {
            Nullable<Length> width = convertValueToLength((*list)[0]);
            if (width.hasValue()) {
                result.m_width = width.getValue();
            }
        }
        if (list->size() >= 2) {
            Nullable<Length> height = convertValueToLength((*list)[1]);
            if (height.hasValue()) {
                result.m_height = height.getValue();
            }
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return result;
}

static AnimatedValue* backgroundSizeToAnimatedValue(
    ComputedStyle* style, Element* element, const CSSStyleValuePair& property,
    size_t layer)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() > layer) {
            return new AnimatedValue(
                backgroundSizeToLengthSize((*list)[layer]));
        } else {
            return new AnimatedValue(
                backgroundSizeToLengthSize((*list)[list->size() - 1]));
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return nullptr;
}

static AnimatedValue* animatedColorValue(const CSSStyleValuePair& property)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
        return new AnimatedValue(property.colorValue());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::NamedColorValueKind) {
        return new AnimatedValue(
            NamedColor::namedColorToColor(property.namedColorValue()));
    } else {
        // TODO: Consider how to handle in this case.
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return new AnimatedValue(Unit::Color(0, 0, 0, 0));
    }
}

static AnimatedValue* animatedLengthValue(const CSSStyleValuePair& property)
{
    if (property.valueKind() == CSSStyleValuePair::ValueKind::Length) {
        return new AnimatedValue(property.lengthValue());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::CalcValueKind) {
        return new AnimatedValue(property.lengthValue());
    } else if (property.valueKind() == CSSStyleValuePair::ValueKind::Auto) {
        return new AnimatedValue(Length());
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::Percentage) {
        return new AnimatedValue(
            Length(Length::Percent, property.percentageValue()));
    } else {
        // TODO: Consider how to handle in this case.
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return new AnimatedValue(Length(Length::Fixed, 0));
    }
}

static AnimatedValue* animatedValue(ComputedStyle* style, Element* element,
                                    const CSSStyleValuePair& property,
                                    const CSSStyleValuePair::KeyKind& keyKind,
                                    size_t layer = 0,
                                    bool neededOriginProperty = false)
{
    STARFISH_ASSERT(style != nullptr);

    switch (keyKind) {
    case CSSStyleValuePair::Color:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BackgroundColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->backgroundColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderBottomColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().bottom().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderLeftColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().left().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderRightColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().right().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::BorderTopColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().top().color());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::CaretColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->caretColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::OutlineColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->outlineColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::TextDecorationColor:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->textDecorationColor());
        }
        return animatedColorValue(property);
    case CSSStyleValuePair::Width:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MaxWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->maxWidth());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MinWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->minWidth());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginTop:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().top());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginRight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().right());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginBottom:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().bottom());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MarginLeft:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->margin().left());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderTopWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().top().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderRightWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().right().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderBottomWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().bottom().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::BorderLeftWidth:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->border().left().width());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingTop:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().top());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingRight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().right());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingBottom:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().bottom());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::PaddingLeft:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->padding().left());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Height:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->height());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MaxHeight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->maxHeight());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::MinHeight:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->minHeight());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Left:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->left());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Right:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->right());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Top:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->top());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::Bottom:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->bottom());
        }
        return animatedLengthValue(property);
    case CSSStyleValuePair::FontSize:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->fontSize());
        }
        return animatedLengthValue(property);
        break;
    case CSSStyleValuePair::BackgroundPositionX:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->backgroundPositionX(layer));
        }

        if (style->backgroundLayerSize() > 0) {
            return backgroundPositionToAnimatedValue(style, element, property,
                                                     layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::BackgroundPositionY:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->backgroundPositionY(layer));
        }

        if (style->backgroundLayerSize() > 0) {
            return backgroundPositionToAnimatedValue(style, element, property,
                                                     layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::BackgroundSize:
        if (style->backgroundLayerSize() > 0) {
            if (neededOriginProperty == true) {
                return new AnimatedValue(
                    style->backgroundSizeLengthValue(layer));
            }
            return backgroundSizeToAnimatedValue(style, element, property,
                                                 layer);
        } else {
            // TODO: Consider how to handle in this case.
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        break;
    case CSSStyleValuePair::Opacity:
        if (neededOriginProperty == true) {
            return new AnimatedValue(style->opacity());
        }

        if (property.valueKind() == CSSStyleValuePair::ValueKind::Number) {
            return new AnimatedValue(property.numberValue());
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            return nullptr;
        }
        break;
    case CSSStyleValuePair::Transform:
        if (neededOriginProperty == true) {
            StyleTransformDataGroup* transform = style->transforms();
            if (transform) {
                return new AnimatedValue(transform);
            } else {
                return new AnimatedValue(new StyleTransformDataGroup());
            }
        }

        if (property.valueKind() ==
            CSSStyleValuePair::ValueKind::TransformFunctions) {
            auto transformValue = property.transformValue();
            ComputedStyle receiver(style);
            transformValue->toTransformDataGroup(&receiver);
            STARFISH_ASSERT(receiver.transforms() != nullptr);
            return new AnimatedValue(receiver.transforms());
        } else {
            return nullptr;
        }
        break;
    default:
        break;
    }

    return nullptr;
}

bool applyAnimationIfNeeds(Element* element, ComputedStyle* style,
                           bool isCSSAnimationTask)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);

    bool ret = false;

    AnimationExecutor* executor = element->document()->animationExecutor();

    if (style->animation() == nullptr) {
        return false;
    }
    StyleAnimationData* animation = style->animation();
    size_t keyframesSize = animation->keyframesSize();
    for (size_t s = 0; s < keyframesSize; s++) {
        String* name = animation->animationName(s);
        if (name->equals(String::emptyString) == true ||
            name->equalsIgnoreCase("none") == true) {
            continue;
        }

        double duration = animation->duration(s).toTimeValue();
        if (duration == 0.0) {
            continue;
        }

        AnimationKeyframes& keyframes = animation->keyframes(s);
        if (keyframes.keyframeList().size() == 0) {
            continue;
        }

        AnimationKeyframe* fromKeyframe = keyframes.keyframe(0);
        if (fromKeyframe == nullptr) {
            continue;
        }

        double delay = animation->delay(s).toTimeValue();
        float iterationCount = animation->iterationCount(s);
        AnimationDirectionValue direction = animation->direction(s);
        AnimationPlayStateValue playState = animation->playState(s);
        AnimationFillModeValue fillMode = animation->fillMode(s);

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
                layerSize = style->backgroundLayerSize();
            }

            GCVector<GCVector<AnimatedValue*>> values;
            values.resize(layerSize);
            bool isAvailable = true;
            for (size_t l = 0; l < layerSize; l++) {
                AnimatedValue* value = animatedValue(
                    style, element, property, keyKind, l, neededOriginProperty);
                if (value == nullptr) {
                    isAvailable = false;
                    break;
                }
                values[l].push_back(value);
            }
            if (isAvailable == false) {
                continue;
            }

            GCAtomicVector<double> offsets;
            GCVector<TimingFunction*> timingFunctions;
            offsets.push_back(fromKeyframe->keyframeName());
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
                    AnimatedValue* value =
                        animatedValue(style, element, property, keyKind, l,
                                      neededOriginProperty);
                    if (value == nullptr) {
                        isAvailable = false;
                        break;
                    }
                    values[l].push_back(value);
                }
                if (isAvailable == false) {
                    continue;
                }

                offsets.push_back(keyframe->keyframeName());
                timingFunctions.push_back(keyframe->timingFunction());
            }

            bool gotAnimation = false;

            // color series
            if (CHECK_ANIMATION(CSSStyleValuePair::BackgroundColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BackgroundColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BackgroundColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::BorderBottomColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderBottomColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderBottomColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::BorderLeftColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderLeftColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderLeftColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::BorderRightColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderRightColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderRightColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::BorderTopColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::BorderTopColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderTopColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::Color) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::Color, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::Color);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::CaretColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::CaretColor, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::CaretColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::OutlineColor) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::OutlineColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::OutlineColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::TextDecorationColor,
                                CSSStyleValuePair::TextDecoration) == true) {
                auto task = new ActiveColorAnimationTask(
                    element, CSSStyleValuePair::TextDecorationColor, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::TextDecorationColor);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }
// <- color series

// length series

#define APPLY_LENGTH_ANIMATION(propertyName)                                   \
    if (keyKind == CSSStyleValuePair::propertyName) {                          \
        auto task = new ActiveLengthAnimationTask(                             \
            element, CSSStyleValuePair::propertyName, values[0], offsets,      \
            timingFunctions, duration, delay, iterationCount, playState,       \
            fillMode);                                                         \
        executor->removeActiveAnimationTaskIfNeeds(                            \
            element, CSSStyleValuePair::propertyName);                         \
        executor->registerAnimation(task, style, name, s, iterationCount,      \
                                    direction, playState, isCSSAnimationTask); \
        gotAnimation = true;                                                   \
    }

            APPLY_LENGTH_ANIMATION(Width)
            APPLY_LENGTH_ANIMATION(Height)
            APPLY_LENGTH_ANIMATION(MinWidth)
            APPLY_LENGTH_ANIMATION(MaxWidth)
            APPLY_LENGTH_ANIMATION(MinHeight)
            APPLY_LENGTH_ANIMATION(MaxHeight)

#undef APPLY_LENGTH_ANIMATION

            if (CHECK_ANIMATION(CSSStyleValuePair::MarginTop) == true) {
                if (style->display() == DisplayValue::InlineDisplayValue) {
                    continue;
                }
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginTop, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::MarginTop);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::MarginRight) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginRight, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::MarginRight);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::MarginBottom) == true) {
                if (style->display() == DisplayValue::InlineDisplayValue) {
                    continue;
                }
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginBottom, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::MarginBottom);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::MarginLeft) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::MarginLeft, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::MarginLeft);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::BorderTopWidth) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderTopWidth, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderTopWidth);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::BorderRightWidth) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderRightWidth, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderRightWidth);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::BorderBottomWidth) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderBottomWidth, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderBottomWidth);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::BorderLeftWidth) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::BorderLeftWidth, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::BorderLeftWidth);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::PaddingTop) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingTop, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::PaddingTop);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::PaddingRight) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingRight, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::PaddingRight);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::PaddingBottom) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingBottom, values[0],
                    offsets, timingFunctions, duration, delay, iterationCount,
                    playState, fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::PaddingBottom);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::PaddingLeft) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::PaddingLeft, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::PaddingLeft);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

#define APPLY_SIDE_ANIMATION(propertyName)                                     \
    if (CHECK_ANIMATION(CSSStyleValuePair::propertyName) == true) {            \
        auto task = new ActiveLengthAnimationTask(                             \
            element, CSSStyleValuePair::propertyName, values[0], offsets,      \
            timingFunctions, duration, delay, iterationCount, playState,       \
            fillMode);                                                         \
        executor->removeActiveAnimationTaskIfNeeds(                            \
            element, CSSStyleValuePair::propertyName);                         \
        executor->registerAnimation(task, style, name, s, iterationCount,      \
                                    direction, playState, isCSSAnimationTask); \
        gotAnimation = true;                                                   \
    }
            APPLY_SIDE_ANIMATION(Left)
            APPLY_SIDE_ANIMATION(Right)
            APPLY_SIDE_ANIMATION(Top)
            APPLY_SIDE_ANIMATION(Bottom)

#undef APPLY_SIDE_ANIMATION

            if (CHECK_ANIMATION(CSSStyleValuePair::BackgroundPositionX) ==
                true) {
                if (style->hasBlockLikeDisplay() == false) {
                    // TODO Inline Element
                    continue;
                }
                for (size_t l = 0; l < layerSize; l++) {
                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionX,
                        values[l], offsets, timingFunctions, duration, delay,
                        iterationCount, playState, fillMode, l);
                    executor->removeActiveAnimationTaskIfNeeds(
                        element, CSSStyleValuePair::BackgroundPositionX, l);
                    executor->registerAnimation(task, style, name, s,
                                                iterationCount, direction,
                                                playState, isCSSAnimationTask);
                }

                gotAnimation = true;
            }
            if (CHECK_ANIMATION(CSSStyleValuePair::BackgroundPositionY) ==
                true) {
                if (style->hasBlockLikeDisplay() == false) {
                    // TODO Inline Element
                    continue;
                }
                for (size_t l = 0; l < layerSize; l++) {
                    auto task = new ActiveLengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionY,
                        values[l], offsets, timingFunctions, duration, delay,
                        iterationCount, playState, fillMode, l);
                    executor->removeActiveAnimationTaskIfNeeds(
                        element, CSSStyleValuePair::BackgroundPositionY, l);
                    executor->registerAnimation(task, style, name, s,
                                                iterationCount, direction,
                                                playState, isCSSAnimationTask);
                }
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::BackgroundSize) == true) {
                if (style->hasBlockLikeDisplay() == false) {
                    // TODO Inline Element
                    continue;
                }
                for (size_t l = 0; l < layerSize; l++) {
                    auto task = new ActiveLengthSizeAnimationTask(
                        element, CSSStyleValuePair::BackgroundSize, values[l],
                        offsets, timingFunctions, duration, delay,
                        iterationCount, playState, fillMode, l);
                    executor->removeActiveAnimationTaskIfNeeds(
                        element, CSSStyleValuePair::BackgroundSize, l);
                    executor->registerAnimation(task, style, name, s,
                                                iterationCount, direction,
                                                playState, isCSSAnimationTask);
                }
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::FontSize,
                                CSSStyleValuePair::Font) == true) {
                auto task = new ActiveLengthAnimationTask(
                    element, CSSStyleValuePair::FontSize, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::FontSize);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::Opacity) == true) {
                auto task = new ActiveOpacityAnimationTask(
                    element, CSSStyleValuePair::Opacity, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::Opacity);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (CHECK_ANIMATION(CSSStyleValuePair::Transform) == true) {
                auto task = new ActiveTransformAnimationTask(
                    element, CSSStyleValuePair::Transform, values[0], offsets,
                    timingFunctions, duration, delay, iterationCount, playState,
                    fillMode);
                executor->removeActiveAnimationTaskIfNeeds(
                    element, CSSStyleValuePair::Transform);
                executor->registerAnimation(task, style, name, s,
                                            iterationCount, direction,
                                            playState, isCSSAnimationTask);
                gotAnimation = true;
            }

            if (gotAnimation == true) {
                // TODO reduce animation duration here with
                // canceledAnimationProgress
                ret = true;
            }
        }

        if (ret == true) {
            executor->fireAnimationStartEvent(element, name, delay);
        }
    }

    return ret;
}

size_t ActiveElementAnimation::hashValue() const
{
    if (m_hash == 0) {
        hash_combine(m_hash, m_name->hashValue());
        hash_combine(m_hash, (size_t)m_element);
    }
    return m_hash;
}

bool ActiveElementAnimation::equals(const ActiveElementAnimation* src) const
{
    STARFISH_ASSERT(src != nullptr);
    return (m_name->equals(src->m_name) == true) &&
           (m_element == src->m_element);
}
}
