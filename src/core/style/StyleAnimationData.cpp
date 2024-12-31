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

#include "StarfishConfig.h"
#include "core/animation/CubicBezier.h"
#include "core/animation/TimingFunction.h"
#include "core/style/Style.h"
#include "core/style/StyleAnimationData.h"

namespace Starfish {
TimingFunction* AnimationKeyframe::defaultTimingFunction()
{
    return CubicBezier::createCubicBezier(CubicBezierEaseType::Ease);
}

bool StyleAnimationData::isValidToApply(size_t index) const
{
    if (animationName(index)->equals(String::emptyString) == true ||
        animationName(index)->equalsIgnoreCase("none") == true) {
        return false;
    }

    if (duration(index).toTimeValue() == 0.0) {
        return false;
    }

    if (!m_animationKeyframesList[index].animationKeyframeListSize()) {
        return false;
    }

    return true;
}

} // namespace Starfish
