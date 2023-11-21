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

#include "core/style/BorderDirectionAwereData.h"
#include "core/style/Style.h"

namespace Starfish {

bool BorderDirectionAwereData::damaged(const BorderDirectionAwereData* lhs,
                                       const BorderDirectionAwereData* rhs,
                                       bool* damagedKeys)
{
    if (!lhs && !rhs) {
        return false;
    }

    damagedKeys[CSSStyleValuePair::KeyKind::BorderBlockStartColor] = false;

    BorderDirectionAwereData temp;
    lhs = lhs ? lhs : &temp;
    rhs = rhs ? rhs : &temp;
    bool hasDamage = false;

    if (lhs->m_borderValue.hasBorderColor() !=
            rhs->m_borderValue.hasBorderColor() ||
        lhs->m_borderValue.color() != rhs->m_borderValue.color()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderBlockStartColor] =
            hasDamage = true;
    }
    if (lhs->m_borderValue.style() != rhs->m_borderValue.style()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderBlockStartStyle] =
            hasDamage = true;
    }
    if (lhs->m_borderValue.width() != rhs->m_borderValue.width()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderBlockStartWidth] =
            hasDamage = true;
    }

    return hasDamage;
}

} // namespace Starfish
