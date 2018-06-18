/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishAnimationUtil__
#define __StarFishAnimationUtil__

namespace StarFish {

class ComputedStyle;
class FrameBox;

class AnimationUtil {
public:
    static bool backgroundSizeToAnimatedValue(
        ComputedStyle* oldStyle, ComputedStyle* newStyle,
        FrameBox* oldPaintingBox, Element* element, AnimatedValue& from,
        AnimatedValue& to, uint32_t layer);
    static bool backgroundPosXToAnimatedValue(
        ComputedStyle* oldStyle, ComputedStyle* newStyle,
        FrameBox* oldPaintingBox, Element* element, AnimatedValue& from,
        AnimatedValue& to, uint32_t layer);
    static bool backgroundPosYToAnimatedValue(
        ComputedStyle* oldStyle, ComputedStyle* newStyle,
        FrameBox* oldPaintingBox, Element* element, AnimatedValue& from,
        AnimatedValue& to, uint32_t layer);
    static bool marginTopToAnimatedValue(ComputedStyle* oldStyle,
                                         ComputedStyle* newStyle,
                                         Element* element, AnimatedValue& from,
                                         AnimatedValue& to);
    static bool marginRightToAnimatedValue(ComputedStyle* oldStyle,
                                           ComputedStyle* newStyle,
                                           Element* element,
                                           AnimatedValue& from,
                                           AnimatedValue& to);
    static bool marginBottomToAnimatedValue(ComputedStyle* oldStyle,
                                            ComputedStyle* newStyle,
                                            Element* element,
                                            AnimatedValue& from,
                                            AnimatedValue& to);
    static bool marginLeftToAnimatedValue(ComputedStyle* oldStyle,
                                          ComputedStyle* newStyle,
                                          Element* element, AnimatedValue& from,
                                          AnimatedValue& to);
};
}

#endif
