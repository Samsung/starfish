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

#ifndef __StarfishAnimationTask__
#define __StarfishAnimationTask__

#include "core/dom/EventTarget.h"
#include "core/style/Style.h"
#include "core/style/StyleBackgroundData.h"
#include "core/animation/AnimatedValue.h"

namespace Starfish {

class Node;
class StyleTransformDataGroup;
class TimingFunction;

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* oldStyle, Frame* oldFrame,
    ComputedStyle* newStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, double>>&
        canceledAnimationProgress); // returns true if animation registered

bool applyAnimationIfNeeds(Element* element, ComputedStyle* style,
                           bool isCSSAnimationTask = true);

class ActiveAnimationTask : public gc {
public:
    ActiveAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        const AnimatedValue& from, const AnimatedValue& to,
                        uint64_t durationInms, int64_t delayInms,
                        TimingFunction* timingFunction);

    ActiveAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        const GCVector<AnimatedValue*>& animatedValues,
                        const GCAtomicVector<double>& keyframeNames,
                        const GCVector<TimingFunction*>& timingFunctions,
                        uint64_t durationInms, int64_t delayInms,
                        float iterationCount, AnimationPlayStateValue playState,
                        AnimationFillModeValue fillMode);

    virtual ~ActiveAnimationTask()
    {
    }

    CSSStyleValuePair::KeyKind property() const
    {
        return m_property;
    }

    Element* targetElement() const
    {
        return m_targetElement;
    }

    void step(uint64_t tickCount, ComputedStyle* style);

    virtual void execute(double progress, ComputedStyle* style)
    {
    }

    virtual bool taskCanContinue(ComputedStyle* newStyle)
    {
        return false;
    }

    virtual void attachToElement()
    {
    }

    virtual void detachFromElement()
    {
    }

    virtual bool isKindOfTransitionProperty(CSSStyleValuePair::KeyKind key)
    {
        return key == m_property;
    }

    virtual void resolveUnresolvedAnimatedValues()
    {
        m_isEveryAnimiatedValueResolved = true;
    }

    virtual void didAnimationFrameChanged()
    {
    }

    virtual size_t backgroundLayer()
    {
        return 0;
    }

    double fraction(uint64_t tickCount) const
    {
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

    uint64_t remainTime(uint64_t tickCount) const
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

    bool isForward()
    {
        return m_isForward;
    }

    void setIsForward(bool isForward)
    {
        if (m_isForward == true && isForward == false) {
            m_frameIdx = m_frameSize - 1;
        } else if (m_isForward == false && isForward == true) {
            m_frameIdx = 0;
        }
        m_isForward = isForward;
    }

    bool isCSSAnimationTask()
    {
        return m_isCSSAnimationTask;
    }

    void setIsCSSAnimationTask(bool b)
    {
        m_isCSSAnimationTask = b;
    }

    bool isInForwardsFillMode()
    {
        return m_isInForwardsFillMode;
    }

    void markInForwardsFillMode()
    {
        m_isInForwardsFillMode = true;
    }

    void clearInForwardsFillMode()
    {
        m_isInForwardsFillMode = false;
    }

    void fireTransitionStartEvent();
    void fireTransitionEndEvent();
    void fireTransitionCancelEvent();

    void updateDuration(uint64_t d)
    {
        m_durationMs = d;
    }

    uint64_t duration()
    {
        return m_durationMs;
    }

    void initializeStartTimeIfNeeded(uint64_t d)
    {
        if (m_startTimeMs == 0) {
            m_startTimeMs = d;
        }
    }

    void setStartTime(uint64_t t)
    {
        STARFISH_ASSERT(m_startTimeMs);
        m_startTimeMs = t;
    }

    uint64_t startTime()
    {
        STARFISH_ASSERT(m_startTimeMs);
        return m_startTimeMs;
    }

    void setGapTime(uint64_t t)
    {
        m_gapTimeMs = t;
    }

    uint64_t gapTime()
    {
        return m_gapTimeMs;
    }

    void setIsRunning(bool isRunning)
    {
        m_isRunning = isRunning;
    }

    enum TYPE ENSURE_ENUM_UNSIGNED { TRANSITION_TYPE, ANIMATION_TYPE };
    TYPE type()
    {
        return m_type;
    }

    float iterationStart()
    {
        return m_iterationStart;
    }

    void setIterationStart(float f)
    {
        if (std::isinf(f) == true) {
            m_iterationStart = 1;
        } else {
            m_iterationStart = f;
        }
    }

    AnimationPlayStateValue playState()
    {
        return m_playState;
    }

    void setPlayState(AnimationPlayStateValue v)
    {
        m_playState = v;
    }

    AnimationFillModeValue fillMode()
    {
        return m_fillMode;
    }

    void setFillMode(AnimationFillModeValue v)
    {
        m_fillMode = v;
    }

    size_t currentAnimatedFromFrameIndex()
    {
        return m_frameIdx;
    }

    size_t currentAnimatedToFrameIndex()
    {
        if (m_isForward) {
            return m_frameIdx + 1;
        } else {
            return m_frameIdx - 1;
        }
    }

    AnimatedValue* currentAnimatedFromValue();
    AnimatedValue* currentAnimatedToValue();
    TimingFunction* currentTimingFunction();

    const GCVector<AnimatedValue*>& values() const
    {
        return m_values;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        STARFISH_ASSERT(desc != nullptr);
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveAnimationTask, m_targetElement));
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveAnimationTask, m_values));
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveAnimationTask, m_offsets));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(ActiveAnimationTask, m_timingFunctions));
    }

    double computeProgress(double& fraction);

    bool m_isEveryAnimiatedValueResolved : 1;
    TYPE m_type : 1;
    CSSStyleValuePair::KeyKind m_property : 8;

    Element* m_targetElement;

    uint64_t m_startTimeMs;
    uint64_t m_durationMs;
    int64_t m_startDelayMs;
    int64_t m_delayMs; // delays can be negative
    AnimationPlayStateValue m_playState;
    AnimationFillModeValue m_fillMode;
    uint64_t m_gapTimeMs;
    float m_iterationCount;
    float m_iterationStart;
    bool m_isInDelayedTime;
    bool m_isForward;
    bool m_isRunning;
    bool m_isCSSAnimationTask;
    bool m_isInForwardsFillMode;

    unsigned int m_frameIdx;
    unsigned int m_frameSize;

    GCVector<AnimatedValue*> m_values;
    GCAtomicVector<double> m_offsets;
    GCVector<TimingFunction*> m_timingFunctions;
};

class ActiveOpacityAnimationTask : public ActiveAnimationTask {
public:
    ActiveOpacityAnimationTask(Element* target,
                               CSSStyleValuePair::KeyKind targetProperty,
                               const AnimatedValue& from,
                               const AnimatedValue& to, uint64_t durationInms,
                               int64_t delayInms,
                               TimingFunction* timingFunction)
        : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                              delayInms, timingFunction)
    {
        STARFISH_ASSERT(target != nullptr);
        STARFISH_ASSERT(timingFunction != nullptr);
    }

    ActiveOpacityAnimationTask(Element* target,
                               CSSStyleValuePair::KeyKind targetProperty,
                               const GCVector<AnimatedValue*>& values,
                               const GCAtomicVector<double>& offsets,
                               const GCVector<TimingFunction*>& timingFunctions,
                               uint64_t durationInms, int64_t delayInms,
                               float iterationCount,
                               AnimationPlayStateValue playState,
                               AnimationFillModeValue fillMode);

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual void attachToElement() override;
    virtual void detachFromElement() override;
};

class ActiveTransformAnimationTask : public ActiveAnimationTask {
public:
    struct MatrixDecomposed2D {
        float translateX;
        float translateY;
        float scaleX;
        float scaleY;
        float angle;
        float matrixM11;
        float matrixM12;
        float matrixM21;
        float matrixM22;

        MatrixDecomposed2D()
            : translateX(0)
            , translateY(0)
            , scaleX(0)
            , scaleY(0)
            , angle(0)
            , matrixM11(0)
            , matrixM12(0)
            , matrixM21(0)
            , matrixM22(0)

        {
        }
    };

    ActiveTransformAnimationTask(Element* target,
                                 CSSStyleValuePair::KeyKind targetProperty,
                                 const AnimatedValue& from,
                                 const AnimatedValue& to, uint64_t durationInms,
                                 int64_t delayInms,
                                 TimingFunction* timingFunction,
                                 StyleTransformDataGroup* orgTransformValue);

    ActiveTransformAnimationTask(
        Element* target, CSSStyleValuePair::KeyKind targetProperty,
        const GCVector<AnimatedValue*>& values,
        const GCAtomicVector<double>& offsets,
        const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
        int64_t delayInms, float iterationCount,
        AnimationPlayStateValue playState, AnimationFillModeValue fillMode);

    virtual void resolveUnresolvedAnimatedValues() override;
    virtual void didAnimationFrameChanged() override;

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual void attachToElement() override;
    virtual void detachFromElement() override;

    const MatrixDecomposed2D& decomposedFrom()
    {
        return m_decomposedFrom;
    }

    const MatrixDecomposed2D& decomposedTo()
    {
        return m_decomposedTo;
    }

    static inline void fillGCDescriptor(GC_word* desc)
    {
        ActiveAnimationTask::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveTransformAnimationTask,
                                        m_originalTransformValue));
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveTransformAnimationTask,
                                        m_fromTransformValue));
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveTransformAnimationTask,
                                        m_toTransformValue));
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    bool needsDecompositing(StyleTransformDataGroup* from,
                            StyleTransformDataGroup* to);
    void resolveTransformValues();
    void removePercentValuesFromTransform();

    // https://www.w3.org/TR/css-transforms-1/#interpolation-of-transforms
    // Two transform functions with the same name and the same number of
    // arguments are interpolated numerically
    // without a former conversion
    bool m_shouldUseDecompositing;

    StyleTransformDataGroup* m_originalTransformValue;
    MatrixDecomposed2D m_decomposedFrom;
    MatrixDecomposed2D m_decomposedTo;
    StyleTransformDataGroup* m_fromTransformValue;
    StyleTransformDataGroup* m_toTransformValue;
};

class ActiveColorAnimationTask : public ActiveAnimationTask {
public:
    ActiveColorAnimationTask(Element* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             const AnimatedValue& from, const AnimatedValue& to,
                             uint64_t durationInms, int64_t delayInms,
                             TimingFunction* timingFunction)
        : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                              delayInms, timingFunction)
    {
        STARFISH_ASSERT(target != nullptr);
        STARFISH_ASSERT(timingFunction != nullptr);
    }

    ActiveColorAnimationTask(Element* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             const GCVector<AnimatedValue*>& values,
                             const GCAtomicVector<double>& offsets,
                             const GCVector<TimingFunction*>& timingFunctions,
                             uint64_t durationInms, int64_t delayInms,
                             float iterationCount,
                             AnimationPlayStateValue playState,
                             AnimationFillModeValue fillMode)
        : ActiveAnimationTask(target, targetProperty, values, offsets,
                              timingFunctions, durationInms, delayInms,
                              iterationCount, playState, fillMode)
    {
        m_isEveryAnimiatedValueResolved = true;
        STARFISH_ASSERT(target != nullptr);
    }

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind key) override;
};

class ActiveLengthAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthAnimationTask(Element* target,
                              CSSStyleValuePair::KeyKind targetProperty,
                              const AnimatedValue& from,
                              const AnimatedValue& to, uint64_t durationInms,
                              int64_t delayInms, TimingFunction* timingFunction,
                              Length originalToValue,
                              size_t indexForBgLayer = 0);

    ActiveLengthAnimationTask(Element* target,
                              CSSStyleValuePair::KeyKind targetProperty,
                              const GCVector<AnimatedValue*>& values,
                              const GCAtomicVector<double>& offsets,
                              const GCVector<TimingFunction*>& timingFunctions,
                              uint64_t durationInms, int64_t delayInms,
                              float iterationCount,
                              AnimationPlayStateValue playState,
                              AnimationFillModeValue fillMode,
                              size_t indexForBgLayer = 0);

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind key) override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        STARFISH_ASSERT(desc != nullptr);
        ActiveAnimationTask::fillGCDescriptor(desc);
        GC_set_bit(
            desc, GC_WORD_OFFSET(ActiveLengthAnimationTask, m_originalToValue));
    }

    virtual void resolveUnresolvedAnimatedValues() override;

    virtual size_t backgroundLayer()
    {
        return m_indexForBgLayer;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    Length m_originalToValue;
    size_t m_indexForBgLayer;
};

class ActiveLengthSizeAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthSizeAnimationTask(Element* target,
                                  CSSStyleValuePair::KeyKind targetProperty,
                                  const AnimatedValue& from,
                                  const AnimatedValue& to,
                                  uint64_t durationInms, int64_t delayInms,
                                  TimingFunction* timingFunction,
                                  LengthSize originalToValue,
                                  size_t indexForBgLayer = 0);
    ActiveLengthSizeAnimationTask(
        Element* target, CSSStyleValuePair::KeyKind targetProperty,
        const GCVector<AnimatedValue*>& values,
        const GCAtomicVector<double>& offsets,
        const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
        int64_t delayInms, float iterationCount,
        AnimationPlayStateValue playState, AnimationFillModeValue fillMode,
        size_t indexForBgLayer = 0);

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind key) override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        STARFISH_ASSERT(desc != nullptr);
        ActiveAnimationTask::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveLengthSizeAnimationTask,
                                        m_originalToValue));
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    LengthSize m_originalToValue;
    size_t m_indexForBgLayer;
};

class ActiveVisibilityAnimationTask : public ActiveAnimationTask {
public:
    ActiveVisibilityAnimationTask(Element* target,
                                  CSSStyleValuePair::KeyKind targetProperty,
                                  const AnimatedValue& from,
                                  const AnimatedValue& to,
                                  uint64_t durationInms, int64_t delayInms,
                                  TimingFunction* timingFunction)
        : ActiveAnimationTask(target, targetProperty, from, to, durationInms,
                              delayInms, timingFunction)
    {
        STARFISH_ASSERT(target != nullptr);
        STARFISH_ASSERT(timingFunction != nullptr);
    }

    ActiveVisibilityAnimationTask(
        Element* target, CSSStyleValuePair::KeyKind targetProperty,
        const GCVector<AnimatedValue*>& values,
        const GCAtomicVector<double>& offsets,
        const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
        int64_t delayInms, float iterationCount,
        AnimationPlayStateValue playState, AnimationFillModeValue fillMode)
        : ActiveAnimationTask(target, targetProperty, values, offsets,
                              timingFunctions, durationInms, delayInms,
                              iterationCount, playState, fillMode)
    {
        m_isEveryAnimiatedValueResolved = true;
        STARFISH_ASSERT(target != nullptr);
    }

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
};

} // namespace Starfish

#endif
