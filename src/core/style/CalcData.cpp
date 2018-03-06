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
#include "core/style/CalcData.h"

namespace StarFish {
LayoutUnit CalcValue::specifiedValue(LayoutUnit parentContentLength,
                                     Node* n) const
{
    if (m_type.isLength()) {
        return m_data.m_lengthData.toLength().specifiedValue(
            parentContentLength, n);
    } else if (m_type.isPercentage()) {
        return Length(Length::Percent, m_data.m_numberData)
            .percentValue(parentContentLength);
    }

    return LayoutUnit();
}

LayoutUnit CalcValue::specifiedFontValue(Node* n) const
{
    if (m_type.isLength()) {
        return m_data.m_lengthData.toLength().specifiedFontValue(n);
    } else if (m_type.isPercentage()) {
        return Length(Length::Percent, m_data.m_numberData)
            .specifiedFontValue(n);
    }

    return LayoutUnit();
}
}
