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

enum class AnimationType ENSURE_ENUM_UNSIGNED {
    Transition,
    KeyFramesAnimation,
    WebAnimation,
    SVGAnimation,
};

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* fromStyle, Optional<Frame*> oldFrame,
    ComputedStyle* toStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, double>>&
        canceledAnimationProgress); // returns true if animation registered

struct ActiveAnimationTaskInit {
    AnimationType animationType;
    Element* target;
    CSSStyleValuePair::KeyKind targetProperty;
    AnimationPlayStateValue playState;
    AnimationFillModeValue fillMode;
    uint64_t durationInMs;
    int64_t delayInMs;
    float iterationCount;
    GCAtomicVector<double> offsets; // keyframeNames
    GCVector<TimingFunction*> timingFunctions;
    GCVector<AnimatedValue*> animatedValues;
    Optional<size_t> layerIndex;

    // A child element that adds animation effects to its parent element in SVG.
    Optional<SVGAnimationElement*> originAnimationElement;
};

class ActiveAnimationTask : public gc {
public:
    ActiveAnimationTask(const ActiveAnimationTaskInit& init);

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

    Optional<SVGAnimationElement*> originAnimationElement()
    {
        return m_originAnimationElement;
    }

    void step(uint64_t tickCount, ComputedStyle* style);

    virtual void execute(double progress, ComputedStyle* style) = 0;

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

    virtual size_t layerIndex()
    {
        if (m_layerIndex.hasValue()) {
            return m_layerIndex.getValue();
        }
        return 0;
    }

    double fraction(uint64_t tickCount) const;
    double computeProgress(double& fraction);

    uint64_t remainTime(uint64_t tickCount) const;

    bool isForward()
    {
        return m_isForward;
    }

    void setIsForward(bool isForward);

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

    AnimationType animationType()
    {
        return m_animationType;
    }

    bool isTransition()
    {
        return m_animationType == AnimationType::Transition;
    }

    float iterationCount()
    {
        return m_iterationCount;
    }

    float iterationStart()
    {
        return m_iterationStart;
    }

    void setIterationStart(float f)
    {
        if (std::isinf(f)) {
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

    bool isInDelayedTime()
    {
        return m_isInDelayedTime;
    }

    AnimatedValue* currentAnimatedFromValue();
    AnimatedValue* currentAnimatedToValue();
    TimingFunction* currentTimingFunction();

    const GCVector<AnimatedValue*>& values() const
    {
        return m_values;
    }

    virtual bool needsContinuousRendering(uint64_t tick);

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
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveAnimationTask,
                                        m_originAnimationElement));
    }

    void initialize(const ActiveAnimationTaskInit& init);

    bool m_isEveryAnimiatedValueResolved : 1;
    CSSStyleValuePair::KeyKind m_property : 8;

    AnimationType m_animationType;
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
    bool m_isInForwardsFillMode;

    unsigned int m_frameIdx;
    unsigned int m_frameSize;

    GCVector<AnimatedValue*> m_values;
    GCAtomicVector<double> m_offsets;
    GCVector<TimingFunction*> m_timingFunctions;
    Optional<size_t> m_layerIndex;
    Optional<SVGAnimationElement*> m_originAnimationElement;
};

class ActiveOpacityAnimationTask : public ActiveAnimationTask {
public:
    ActiveOpacityAnimationTask(const ActiveAnimationTaskInit& init);

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

    ActiveTransformAnimationTask(
        const ActiveAnimationTaskInit& init,
        Optional<StyleTransformDataGroup*> originalToValue);

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

    StyleTransformDataGroup* toTransformValue()
    {
        return m_toTransformValue;
    }

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

    Optional<StyleTransformDataGroup*> m_originalTransformValue;
    MatrixDecomposed2D m_decomposedFrom;
    MatrixDecomposed2D m_decomposedTo;
    StyleTransformDataGroup* m_fromTransformValue;
    StyleTransformDataGroup* m_toTransformValue;
};

class ActiveTransformOriginAnimationTask : public ActiveAnimationTask {
public:
    ActiveTransformOriginAnimationTask(const ActiveAnimationTaskInit& init);

    void execute(double progress, ComputedStyle* style) override;
};

class ActiveColorAnimationTask : public ActiveAnimationTask {
public:
    ActiveColorAnimationTask(const ActiveAnimationTaskInit& init);

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind key) override;
};

class ActiveLengthAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthAnimationTask(const ActiveAnimationTaskInit& init,
                              Optional<Length> originalToValue);

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

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    Optional<Length> m_originalToValue;
};

class ActiveLengthSizeAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthSizeAnimationTask(const ActiveAnimationTaskInit& init,
                                  Optional<LengthSize> originalToValue);

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
    Optional<LengthSize> m_originalToValue;
};

class ActiveVisibilityAnimationTask : public ActiveAnimationTask {
public:
    ActiveVisibilityAnimationTask(const ActiveAnimationTaskInit& init);

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
};

class ActiveSVGLengthAnimationTask : public ActiveAnimationTask {
public:
    ActiveSVGLengthAnimationTask(const ActiveAnimationTaskInit& init,
                                 AtomicString attributeName);

    void execute(double progress);
    void end();

    void execute(double progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        ActiveAnimationTask::fillGCDescriptor(desc);
    }

    virtual void attachToElement() override;
    virtual void detachFromElement() override;
    virtual bool needsContinuousRendering(uint64_t tick) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    AtomicString m_attributeName;
};

} // namespace Starfish

#endif
