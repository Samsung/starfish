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

String* CSSGradientValue::toString()
{
    StringBuilder result;
    switch (m_gradientType) {
    case CSSGradientType::LinearGradient: {
        result.appendString("linear-gradient(");
        CSSLinearGradientValue* linear = (CSSLinearGradientValue*)this;
        if (linear->angle().toDegreeValue() != 180) {
            result.appendString(linear->angle().toString());
        }
        // TODO: Consider 'to <side-or-corner>' and <color-stop-list>
        result.appendChar(')');
        break;
    }
    case CSSGradientType::RadialGradient:
        // TODO: Consider radial gradient
        result.appendString("radial-gradient(");
        result.appendChar(')');
        break;
    }
    return result.finalize();
}

} /* namespace StarFish */
