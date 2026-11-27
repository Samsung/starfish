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
class ExecutionContext;
class TimingFunction;
struct KeyframeAnimationOptions;

class TimingOptions {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    static TimingFunction* validate(ExecutionContext* context,
                                    const KeyframeAnimationOptions& options);
    static void apply(Element* element, KeyframeAnimationOptions& options,
                      TimingFunction* easing);
    static Optional<TimingFunction*> parseEasing(String* easing);
};
} // namespace Starfish

#endif
