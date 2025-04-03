/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGAnimationApplier__
#define __StarfishSVGAnimationApplier__

#include "core/style/Style.h"

namespace Starfish {

enum class AnimationType ENSURE_ENUM_UNSIGNED;

class Element;
class TimingFunction;
class AnimatedValue;
class AnimationExecutor;
class AnimationKeyframe;
class AnimationKeyframes;
class ActiveAnimationTask;
class SVGAnimationElement;

class SVGAnimationApplier : public gc {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    SVGAnimationApplier(Element* element,
                        SVGAnimationElement* originAnimationElement);

    bool apply();

private:
    bool applyProperty(String* name, CSSStyleValuePair::KeyKind keyKind,
                       const GCVector<AnimatedValue*>& values,
                       const GCAtomicVector<double>& offsets,
                       const GCVector<TimingFunction*>& timingFunctions,
                       uint64_t duration, int64_t delay, float iterationCount,
                       AnimationDirectionValue direction,
                       AnimationPlayStateValue playState,
                       AnimationFillModeValue fillMode);
    bool createValues(const AnimationKeyframes* currentKeyFrames,
                      CSSStyleValuePair::KeyKind currentKeyKind,
                      size_t currentPropertyIndex,
                      GCVector<AnimatedValue*>& values);
    void createOffsetAndTimingFunction(
        AnimationKeyframes* currentKeyFrames, size_t currentPropertyIndex,
        GCAtomicVector<double>& offsets,
        GCVector<TimingFunction*>& timingFunctions);

    bool isIntermediateDummyAnimationKeyframe(
        AnimationKeyframe* current, CSSStyleValuePair::ValueKind valueKind,
        const AnimationKeyframes* owner);

    void updateActiveAnimationTaskRegistration(
        size_t s, String* name, CSSStyleValuePair::KeyKind keyKind,
        size_t layer, float iterationCount, AnimationDirectionValue direction,
        AnimationPlayStateValue playState, ActiveAnimationTask* task);

    Element* m_element;
    SVGAnimationElement* m_originAnimationElement;

    AnimationExecutor* m_executor;
};

} // namespace Starfish

#endif
