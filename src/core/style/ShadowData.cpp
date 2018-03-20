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
#include "core/style/Style.h"

namespace StarFish {

void ShadowData::setLengths(ValueList* lengths)
{
    STARFISH_ASSERT(2 <= lengths->size() && lengths->size() <= 4);

    m_offsetX = (*lengths)[0].lengthValue();
    m_offsetY = (*lengths)[1].lengthValue();

    if (3 <= lengths->size()) {
        m_radius = (*lengths)[2].lengthValue();
    }
    if (4 == lengths->size()) {
        m_spreadDistance = (*lengths)[3].lengthValue();
    }
}

CanvasShadowData ShadowData::toCanvasShadowData(Frame* owner) const
{
    STARFISH_ASSERT(owner);
    LayoutUnit unused;

    float offsetX = m_offsetX.specifiedValue(unused, owner);
    float offsetY = m_offsetY.specifiedValue(unused, owner);
    float radius = m_radius.specifiedValue(unused, owner);
    float spreadDistance = m_spreadDistance.specifiedValue(unused, owner);

    CanvasShadowData ret(offsetX, offsetY, radius, spreadDistance, m_color,
                         m_hasColor);
    return ret;
}

CanvasShadowDataList ShadowDataList::toCanvasShadowDataList(Frame* owner) const
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
