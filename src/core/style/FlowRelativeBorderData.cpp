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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/style/FlowRelativeBorderData.h"
#include "core/style/Style.h"

namespace Starfish {

const std::array<bool, 3> FlowRelativeBorderData::damaged(
    const FlowRelativeBorderData* lhs, const FlowRelativeBorderData* rhs)
{
    std::array<bool, 3> damages = { false, false, false };

    if (!lhs && !rhs) {
        return damages;
    }

    FlowRelativeBorderData temp;
    lhs = lhs ? lhs : &temp;
    rhs = rhs ? rhs : &temp;

    if (lhs->m_borderValue.hasBorderColor() !=
            rhs->m_borderValue.hasBorderColor() ||
        lhs->m_borderValue.color() != rhs->m_borderValue.color()) {
        damages[static_cast<size_t>(BorderValueKind::kColor)] = true;
    }
    if (lhs->m_borderValue.style() != rhs->m_borderValue.style()) {
        damages[static_cast<size_t>(BorderValueKind::kStyle)] = true;
    }
    if (lhs->m_borderValue.width() != rhs->m_borderValue.width()) {
        damages[static_cast<size_t>(BorderValueKind::kWidth)] = true;
    }

    return damages;
}

} // namespace Starfish
