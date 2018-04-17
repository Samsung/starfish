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

#include "StarFishConfig.h"
#include "core/animation/AnimationTimingFunction.h"
#include "core/animation/CubicBezier.h"
#include "core/animation/Steps.h"

namespace StarFish {
bool AnimationTimingFunction::operator==(const AnimationTimingFunction& b) const
{
    if (isCubicBezier() && b.isCubicBezier()) {
        return *(asCubicBezier()) == *(b.asCubicBezier());
    }
    if (isSteps() && b.isSteps()) {
        return *(asSteps()) == *(b.asSteps());
    }
    return false;
}

bool AnimationTimingFunction::operator!=(const AnimationTimingFunction& b) const
{
    return !operator==(b);
}
}
