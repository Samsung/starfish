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

#ifndef __StarfishAnimation__
#define __StarfishAnimation__

#include "core/style/Style.h"
#include "core/style/StyleBackgroundData.h"

namespace Starfish {

class Node;
class PlatformWindow;
class StyleTransformDataGroup;
class TimingFunction;

class AnimatedValue : public gc {
    enum ValueType ENSURE_ENUM_UNSIGNED {
        UNDEFINED,
        COLOR,
        LAYOUT_UNIT,
        LENGTH,
        LENGTH_SIZE,
        FLOAT,
        INT,
        MATRIX,
        TRANSFORM_DATA,
    };

public:
    AnimatedValue()
    {
        m_type = UNDEFINED;
    }

    AnimatedValue(Unit::Color colorValue)
    {
        m_data.m_color = colorValue;
        m_type = COLOR;
    }

    AnimatedValue(Length lengthValue)
    {
        m_data.m_length = lengthValue;
        m_type = LENGTH;
    }

    AnimatedValue(const LengthSize& size)
    {
        m_data.m_lengthSize = new LengthSize(size);
        m_type = LENGTH_SIZE;
    }

    AnimatedValue(LayoutUnit v)
    {
        m_data.m_layoutUnit = v;
        m_type = LAYOUT_UNIT;
    }

    AnimatedValue(float floatValue)
    {
        m_data.m_float = floatValue;
        m_type = FLOAT;
    }

    AnimatedValue(int intValue)
    {
        m_data.m_int = intValue;
        m_type = INT;
    }

    AnimatedValue(const SkMatrix& matrix)
    {
        m_data.m_matrix = matrix;
        m_type = MATRIX;
    }

    AnimatedValue(StyleTransformDataGroup* transform)
    {
        STARFISH_ASSERT(transform != nullptr);

        m_data.m_transformData = transform;
        m_type = TRANSFORM_DATA;
    }

    bool isColor() const
    {
        return m_type == COLOR;
    }

    bool isLength() const
    {
        return m_type == LENGTH;
    }

    bool isLengthSize() const
    {
        return m_type == LENGTH_SIZE;
    }

    bool isFloat() const
    {
        return m_type == FLOAT;
    }

    bool isInt() const
    {
        return m_type == INT;
    }

    bool isMatrix() const
    {
        return m_type == MATRIX;
    }

    bool isLayoutUnit() const
    {
        return m_type == LAYOUT_UNIT;
    }

    bool isTransformData() const
    {
        return m_type == TRANSFORM_DATA;
    }

    Unit::Color getColor() const
    {
        STARFISH_ASSERT(m_type == COLOR);
        return m_data.m_color;
    }

    Length getLength() const
    {
        STARFISH_ASSERT(m_type == LENGTH);
        return m_data.m_length;
    }

    LengthSize* getLengthSize() const
    {
        STARFISH_ASSERT(m_type == LENGTH_SIZE);
        return m_data.m_lengthSize;
    }

    LayoutUnit getLayoutUnit() const
    {
        STARFISH_ASSERT(m_type == LAYOUT_UNIT);
        return m_data.m_layoutUnit;
    }

    float getFloat() const
    {
        STARFISH_ASSERT(m_type == FLOAT);
        return m_data.m_float;
    }

    int getInt() const
    {
        STARFISH_ASSERT(m_type == INT);
        return m_data.m_int;
    }

    SkMatrix getMatrix() const
    {
        STARFISH_ASSERT(m_type == MATRIX);
        return m_data.m_matrix;
    }

    StyleTransformDataGroup* getTransformData() const
    {
        STARFISH_ASSERT(m_type == TRANSFORM_DATA);
        return m_data.m_transformData;
    }

    inline void* operator new(size_t size, void* p)
    {
        STARFISH_ASSERT(p != nullptr);
        STARFISH_ASSERT(size == sizeof(AnimatedValue));
        return p;
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(AnimatedValue));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(AnimatedValue)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(AnimatedValue, m_data));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(AnimatedValue));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

#ifndef NDEBUG
    String* toString() const;
#endif

protected:
    union ValueData {
        Unit::Color m_color;
        Length m_length;
        LengthSize* m_lengthSize;
        LayoutUnit m_layoutUnit;
        float m_float;
        int m_int;
        SkMatrix m_matrix;
        StyleTransformDataGroup* m_transformData;
        ValueData()
            : m_int(0)
        {
        }
    } m_data;
    ValueType m_type;
};

bool applyTransitionIfNeeds(
    Element* element, ComputedStyle* oldStyle, Frame* oldFrame,
    ComputedStyle* newStyle, const bool* damagedKeys,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, float>>&
        canceledAnimationProgress); // returns true if animation registered

bool applyAnimationIfNeeds(
    Element* element, NULLABLE ComputedStyle* oldStyle,
    NULLABLE Frame* oldFrame, ComputedStyle* newStyle,
    const std::vector<std::pair<CSSStyleValuePair::KeyKind, float>>&
        canceledAnimationProgress);

class ActiveAnimationTask : public gc {
public:
    ActiveAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        const AnimatedValue& from, const AnimatedValue& to,
                        uint64_t durationInms, uint64_t delayInms,
                        TimingFunction* timingFunction);

    ActiveAnimationTask(Element* target,
                        CSSStyleValuePair::KeyKind targetProperty,
                        const GCVector<AnimatedValue*>& animatedValues,
                        const GCAtomicVector<double>& keyframeNames,
                        const GCVector<TimingFunction*>& timingFunctions,
                        uint64_t delayInms, uint64_t durationInms);

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
    virtual void execute(float progress, ComputedStyle* style)
    {
    }
    virtual bool taskCanContinue(ComputedStyle* newStyle)
    {
        return false;
    }

    virtual void attachToElement(ComputedStyle* style)
    {
    }

    virtual void detachFromElement(ComputedStyle* style)
    {
    }

    virtual bool isKindOfTransitionProperty(CSSStyleValuePair::KeyKind k)
    {
        return k == m_property;
    }

    virtual void resolveUnresolvedAnimatedValues()
    {
        m_isEveryAnimiatedValueResolved = true;
    }

    virtual void didAnimationFrameChanged()
    {
        STARFISH_ASSERT(m_type == ANIMATION_TYPE);
    }

    float fraction(uint64_t tickCount) const
    {
        if (tickCount < (m_startTimeMs + m_delayMs)) {
            return 0;
        }
        uint64_t timeDiff = tickCount - (m_startTimeMs + m_delayMs);
        float result = timeDiff / ((float)m_durationMs);

        return std::min(result, 1.0f);
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

    enum TYPE { TRANSITION_TYPE, ANIMATION_TYPE };
    TYPE type()
    {
        return m_type;
    }

    AnimatedValue* currentAnimatedFromValue();
    AnimatedValue* currentAnimatedToValue();
    TimingFunction* currentTimingFunction();

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

    float computeProgress(float& fraction);

    bool m_isEveryAnimiatedValueResolved : 1;
    TYPE m_type : 1;
    CSSStyleValuePair::KeyKind m_property : 8;

    Element* m_targetElement;

    uint64_t m_startTimeMs;
    uint64_t m_durationMs;
    uint64_t m_startDelayMs;
    uint64_t m_delayMs;
    bool m_isInDelayedTime;

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
                               uint64_t delayInms,
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
                               uint64_t durationInms, uint64_t delayInms);

    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual void attachToElement(ComputedStyle* style) override;
    virtual void detachFromElement(ComputedStyle* style) override;
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
                                 uint64_t delayInms,
                                 TimingFunction* timingFunction,
                                 StyleTransformDataGroup* orgTransformValue);

    ActiveTransformAnimationTask(
        Element* target, CSSStyleValuePair::KeyKind targetProperty,
        const GCVector<AnimatedValue*>& values,
        const GCAtomicVector<double>& offsets,
        const GCVector<TimingFunction*>& timingFunctions, uint64_t durationInms,
        uint64_t delayInms);

    virtual void resolveUnresolvedAnimatedValues() override;
    virtual void didAnimationFrameChanged() override;

    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual void attachToElement(ComputedStyle* style) override;
    virtual void detachFromElement(ComputedStyle* style) override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        STARFISH_ASSERT(desc != nullptr);
        ActiveAnimationTask::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(ActiveTransformAnimationTask,
                                        m_originalTransformValue));
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    StyleTransformDataGroup* m_originalTransformValue;
    MatrixDecomposed2D m_decomposedFrom;
    MatrixDecomposed2D m_decomposedTo;
};

class ActiveColorAnimationTask : public ActiveAnimationTask {
public:
    ActiveColorAnimationTask(Element* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             const AnimatedValue& from, const AnimatedValue& to,
                             uint64_t durationInms, uint64_t delayInms,
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
                             uint64_t durationInms, uint64_t delayInms)
        : ActiveAnimationTask(target, targetProperty, values, offsets,
                              timingFunctions, durationInms, delayInms)
    {
        STARFISH_ASSERT(target != nullptr);
    }

    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
};

class ActiveLengthAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthAnimationTask(Element* target,
                              CSSStyleValuePair::KeyKind targetProperty,
                              const AnimatedValue& from,
                              const AnimatedValue& to, uint64_t durationInms,
                              uint64_t delayInms,
                              TimingFunction* timingFunction,
                              Length originalToValue,
                              size_t indexForBgLayer = 0);

    ActiveLengthAnimationTask(Element* target,
                              CSSStyleValuePair::KeyKind targetProperty,
                              const GCVector<AnimatedValue*>& values,
                              const GCAtomicVector<double>& offsets,
                              const GCVector<TimingFunction*>& timingFunctions,
                              uint64_t durationInms, uint64_t delayInms,
                              size_t indexForBgLayer = 0);

    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind k) override;

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
    Length m_originalToValue;
    size_t m_indexForBgLayer;
};

class ActiveLengthSizeAnimationTask : public ActiveAnimationTask {
public:
    ActiveLengthSizeAnimationTask(Element* target,
                                  CSSStyleValuePair::KeyKind targetProperty,
                                  const AnimatedValue& from,
                                  const AnimatedValue& to,
                                  uint64_t durationInms, uint64_t delayInms,
                                  TimingFunction* timingFunction,
                                  LengthSize originalToValue,
                                  size_t indexForBgLayer = 0);
    void execute(float progress, ComputedStyle* style) override;
    virtual bool taskCanContinue(ComputedStyle* newStyle) override;
    virtual bool isKindOfTransitionProperty(
        CSSStyleValuePair::KeyKind k) override;

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

struct ActiveElementAnimation : public gc {
    String* m_name;
    Element* m_element;
    double m_duration;
    double m_delay;
    float m_iterationStart;
    float m_iterationCount;

    ActiveElementAnimation(String* name, Element* element,
                           float iterationCount = 1.0f)
        : m_name(name)
        , m_element(element)
        , m_duration(0)
        , m_delay(0)
        , m_iterationStart(iterationCount)
        , m_iterationCount(iterationCount)
        , m_hash(0)
    {
        STARFISH_ASSERT(name != nullptr);
        STARFISH_ASSERT(element != nullptr);
    }

    size_t hashValue() const;
    bool equals(const ActiveElementAnimation* src) const;

private:
    mutable size_t m_hash;
};
}

namespace std {
template <>
struct hash<Starfish::ActiveElementAnimation*> {
    std::size_t operator()(const Starfish::ActiveElementAnimation* value) const
    {
        return value->hashValue();
    }
};

template <>
struct equal_to<Starfish::ActiveElementAnimation*> {
    bool operator()(const Starfish::ActiveElementAnimation* lhs,
                    const Starfish::ActiveElementAnimation* rhs) const
    {
        return lhs->equals(rhs);
    }
};
}

namespace Starfish {

class AnimationExecutor : public gc {
public:
    AnimationExecutor(Window* window)
        : m_window(window)
    {
        STARFISH_ASSERT(window != nullptr);
        m_window = window;
        m_activeAnimations =
            new GCUnorderedMap<ActiveElementAnimation*,
                               GCVector<ActiveAnimationTask*>,
                               std::hash<ActiveElementAnimation*>,
                               std::equal_to<ActiveElementAnimation*>>();
    }

    Window* window()
    {
        return m_window;
    }

    GCVector<ActiveAnimationTask*>& activeTransitions()
    {
        return m_activeTransitions;
    }

    GCUnorderedMap<ActiveElementAnimation*, GCVector<ActiveAnimationTask*>,
                   std::hash<ActiveElementAnimation*>,
                   std::equal_to<ActiveElementAnimation*>>&
    activeAnimations()
    {
        return *m_activeAnimations;
    }

    void dispose()
    {
        if (m_activeTransitions.size() > 0) {
            m_activeTransitions.clear();
        }
        if (m_activeAnimations != nullptr) {
            for (auto& animations : *m_activeAnimations) {
                animations.second.clear();
            }
            m_activeAnimations->clear();
            m_activeAnimations = nullptr;
        }
    }

    bool hasActiveTransition(Element* element, CSSStyleValuePair::KeyKind p)
    {
        STARFISH_ASSERT(element != nullptr);
        for (size_t i = 0; i < m_activeTransitions.size(); i++) {
            if (m_activeTransitions[i]->targetElement() == element &&
                m_activeTransitions[i]->property() == p) {
                return true;
            }
        }
        return false;
    }

    void registerTransition(ActiveAnimationTask* task, ComputedStyle* style)
    {
        STARFISH_ASSERT(task != nullptr);
        STARFISH_ASSERT(style != nullptr);
        m_activeTransitions.push_back(task);
        task->attachToElement(style);
        task->fireTransitionStartEvent();
    }

    void removeActiveAnimationTaskIfNeeds(Element* element,
                                          CSSStyleValuePair::KeyKind p)
    {
        STARFISH_ASSERT(element != nullptr);

        for (auto animations = m_activeAnimations->begin();
             animations != m_activeAnimations->end(); animations++) {
            for (auto task = (*animations).second.begin();
                 task != (*animations).second.end();) {
                if ((*task)->targetElement() == element &&
                    (*task)->property() == p) {
                    task = (*animations).second.erase(task);
                } else {
                    task++;
                }
            }
        }
    }

    void registerAnimation(ActiveAnimationTask* task, ComputedStyle* style,
                           String* name, float iterationCount)
    {
        STARFISH_ASSERT(task != nullptr);
        STARFISH_ASSERT(style != nullptr);
        STARFISH_ASSERT(name != nullptr);

        task->attachToElement(style);
        ActiveElementAnimation* key = new ActiveElementAnimation(
            name, task->targetElement(), iterationCount);
        auto iter = m_activeAnimations->find(key);
        if (iter == m_activeAnimations->end()) {
            GCVector<ActiveAnimationTask*> v;
            v.push_back(task);
            m_activeAnimations->insert(std::make_pair(key, v));
        } else {
            // Because all of tasks with same property are already removed in
            // removeActiveAnimationTaskIfNeeds(), just add the task to vector.
            iter->second.push_back(task);
        }
    }

    void checkActiveExecutorInWebView();

    void fireAnimationStartEvent(Element* element, String* name, double delay);
    void fireAnimationEndEvent(Element* element, String* name,
                               float elapsedTime);
    void fireAnimationCancelEvent(Element* element, String* name,
                                  float elapsedTime);

private:
    Window* m_window;
    GCVector<ActiveAnimationTask*> m_activeTransitions;
    GCUnorderedMap<ActiveElementAnimation*, GCVector<ActiveAnimationTask*>,
                   std::hash<ActiveElementAnimation*>,
                   std::equal_to<ActiveElementAnimation*>>* m_activeAnimations;
};
}

#endif
