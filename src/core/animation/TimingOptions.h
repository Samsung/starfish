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

#ifndef __StarfishTimingOptions__
#define __StarfishTimingOptions__

namespace Starfish {

class Element;
class TimingFunction;
struct KeyframeAnimationOptions;

enum class AnimationDirectionValue;
enum class AnimationFillModeValue;

struct TimingOutput {
    TimingOutput();

    double m_startDelay;
    double m_endDelay;
    AnimationFillModeValue m_fill;
    double m_iterationStart;
    double m_iterationCount;
    double m_iterationDuration;
    AnimationDirectionValue m_direction;
    TimingFunction* m_easing;
};

class TimingOptions {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    static bool makeTimingOptions(Element* element,
                                  KeyframeAnimationOptions& options);

    static bool setIterationStart(Element* element,
                                  KeyframeAnimationOptions& options,
                                  TimingOutput& output);
    static bool setIterationCount(Element* element,
                                  KeyframeAnimationOptions& options,
                                  TimingOutput& output);
    static bool setIterationDuration(Element* element,
                                     KeyframeAnimationOptions& options,
                                     TimingOutput& output);
    static void setDirection(TimingOutput& output,
                             KeyframeAnimationOptions& options);
    static void setFillMode(TimingOutput& output,
                            KeyframeAnimationOptions& options);
    static bool setTimingFunction(Element* element,
                                  KeyframeAnimationOptions& options,
                                  TimingOutput& output);
};
}

#endif
