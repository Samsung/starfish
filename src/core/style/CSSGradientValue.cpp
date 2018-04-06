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
#include "CSSGradientValue.h"

namespace StarFish {

String* CSSLinearGradientValue::toString()
{
    StringBuilder result;

    result.appendString("linear-gradient(");
    CSSLinearGradientValue* linear = (CSSLinearGradientValue*)this;
    if (linear->angle().toDegreeValue() != 180) {
        result.appendString(linear->angle().toString());
    } else {
        result.appendString("to ");
        if (m_sc & toLeft) {
            result.appendString("left ");
        } else if (m_sc & toRight) {
            result.appendString("right ");
        }

        if (m_sc & toTop) {
            result.appendString("top");
        } else if (m_sc & toBottom) {
            result.appendString("bottom");
        }
    }

    for (auto cs : m_colorStopList) {
        result.appendString(", ");
        result.appendString(cs->color().toString());
        result.appendChar(' ');
        result.appendString(cs->offset().toString());
    }

    result.appendChar(')');

    return result.finalize();
}

} /* namespace StarFish */
