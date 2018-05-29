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
#include "core/style/Style.h"
#include "core/style/StyleTransitionData.h"

namespace StarFish {
StyleTransitionLayer::StyleTransitionLayer()
    : m_property(CSSStyleValuePair::KeyKind::All)
    , m_timingFunction(nullptr)
    , m_duration(0)
    , m_delay(0)
{
}

bool StyleTransitionLayer::operator==(const StyleTransitionLayer& b) const
{
    if (m_property != b.m_property) {
        return false;
    }

    if (m_duration != b.m_duration) {
        return false;
    }

    if (m_delay != b.m_delay) {
        return false;
    }

    if (m_timingFunction != b.m_timingFunction &&
        (!m_timingFunction || !b.m_timingFunction ||
         *m_timingFunction != *b.m_timingFunction)) {
        return false;
    }

    return true;
}

bool StyleTransitionLayer::operator!=(const StyleTransitionLayer& b) const
{
    return !operator==(b);
}

AnimationTimingFunction* StyleTransitionData::defaultTimingFunction()
{
    return new CubicBezier(0.25, 0.1, 0.25, 1);
}

bool StyleTransitionData::operator==(const StyleTransitionData& b) const
{
    if (size() != b.size()) {
        return false;
    }
    size_t len = size();
    for (size_t i = 0; i < len; i++) {
        if (property(i) != b.property(i)) {
            return false;
        }
        if (duration(i) != b.duration(i)) {
            return false;
        }
        AnimationTimingFunction* t1 = timingFunction(i);
        AnimationTimingFunction* t2 = b.timingFunction(i);
        if (t1 != t2 && (!t1 || !t2 || *t1 != *t2)) {
            return false;
        }
        if (delay(i) != b.delay(i)) {
            return false;
        }
    }
    return true;
}
}
