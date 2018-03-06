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

#include "StarFishConfig.h"
#include "ShadowData.h"
#include "core/modules/canvas/CanvasShadowData.h"

namespace StarFish {

CanvasShadowData ShadowData::toCanvasShadowData(Frame* owner)
{
    STARFISH_ASSERT(owner);
    LayoutUnit unused;
    float offsetX = m_offsetX.specifiedValue(unused, owner);
    float offsetY = m_offsetY.specifiedValue(unused, owner);
    float radius = 0.0f;
    if (m_radius.isFixed() && m_radius.fixed()) {
        radius = m_radius.fixed();
    }
    CanvasShadowData ret(offsetX, offsetY, radius, m_color, m_hasColor);
    return ret;
}

CanvasShadowDataList ShadowDataList::toCanvasShadowDataList(Frame* owner)
{
    STARFISH_ASSERT(owner);

    CanvasShadowDataList ret;
    ret.reserve(size());

    for (size_t i = 0; i < size(); i++) {
        ret.push_back(at(i).toCanvasShadowData(owner));
    }
    return ret;
}
}
