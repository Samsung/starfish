/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishTransitionApplier__
#define __StarfishTransitionApplier__

#include "core/style/ComputedStyle.h"

#include <functional>

namespace Starfish {

class Element;
class Frame;
class AnimationExecutor;
class TimingFunction;

class TransitionApplier : public gc {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    TransitionApplier(Element* element, ComputedStyle* oldStyle,
                      Frame* oldFrame, ComputedStyle* newStyle,
                      const bool* damagedKeys);

    bool apply();

private:
    void applyOpacity(double duration, double delay,
                      TimingFunction* timingFunction);

    void applyTransform(double duration, double delay,
                        TimingFunction* timingFunction);

    void applyActiveColorAnimationTask(
        CSSStyleValuePair::KeyKind keyKind,
        const std::function<Unit::Color(ComputedStyle*)>& colorValueGetter,
        double duration, double delay, TimingFunction* timingFunction);
    void applyActiveLengthAnimationTaskForFrameBoxSize(
        CSSStyleValuePair::KeyKind keyKind,
        const std::function<Length(ComputedStyle*)>& lengthValueGetter,
        const std::function<LayoutUnit(FrameBox*)>& frameBoxSizeValueGetter,
        const std::function<LayoutUnit(FrameBox*)>& contentSizeValueGetter,
        double duration, double delay, TimingFunction* timingFunction);

    void applyMarginTop(double duration, double delay,
                        TimingFunction* timingFunction);

    void applyMarginRight(double duration, double delay,
                          TimingFunction* timingFunction);

    void applyMarginBottom(double duration, double delay,
                           TimingFunction* timingFunction);

    void applyMarginLeft(double duration, double delay,
                         TimingFunction* timingFunction);

    void applyBorderTop(double duration, double delay,
                        TimingFunction* timingFunction);

    void applyBorderRight(double duration, double delay,
                          TimingFunction* timingFunction);

    void applyBorderBottom(double duration, double delay,
                           TimingFunction* timingFunction);

    void applyBorderLeft(double duration, double delay,
                         TimingFunction* timingFunction);

    void applyPaddingTop(double duration, double delay,
                         TimingFunction* timingFunction);

    void applyPaddingRight(double duration, double delay,
                           TimingFunction* timingFunction);

    void applyPaddingBottom(double duration, double delay,
                            TimingFunction* timingFunction);

    void applyPaddingLeft(double duration, double delay,
                          TimingFunction* timingFunction);

    void applyLeft(double duration, double delay,
                   TimingFunction* timingFunction);

    void applyRight(double duration, double delay,
                    TimingFunction* timingFunction);

    void applyTop(double duration, double delay,
                  TimingFunction* timingFunction);

    void applyBottom(double duration, double delay,
                     TimingFunction* timingFunction);

    void applyBackgroundPositionX(double duration, double delay,
                                  TimingFunction* timingFunction);

    void applyBackgroundPositionY(double duration, double delay,
                                  TimingFunction* timingFunction);

    void applyBackgroundSize(double duration, double delay,
                             TimingFunction* timingFunction);

    void applyFontSize(double duration, double delay,
                       TimingFunction* timingFunction);

    void applyVisibility(double duration, double delay,
                         TimingFunction* timingFunction);

    void applyProperty(CSSStyleValuePair::KeyKind property, double duration,
                       double delay, TimingFunction* timingFunction);
    void applyAll(double duration, double delay,
                  TimingFunction* timingFunction);
    bool canRegisterTransition(CSSStyleValuePair::KeyKind property);

    Element* m_element;
    ComputedStyle* m_oldStyle;
    Frame* m_oldFrame;
    ComputedStyle* m_newStyle;
    const bool* m_damagedKeys;
    AnimationExecutor* m_executor;
    bool m_gotTransition;
};

} // namespace Starfish

#endif
