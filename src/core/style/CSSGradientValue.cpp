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

bool CSSLinearGradientValue::computeEndPoints(const int width, const int height,
                                              float& x1, float& y1, float& x2,
                                              float& y2)
{
    if (m_sc == 0) {
        float angle = m_angle.toDegreeValue();

        angle = fmodf(angle, 360);
        if (angle < 0)
            angle += 360;

        if (!angle) {
            x1 = 0;
            y1 = height;
            x2 = 0;
            y2 = 0;
            return true;
        }

        if (angle == 90) {
            x1 = 0;
            y1 = 0;

            x2 = width;
            y2 = 0;
            return true;
        }

        if (angle == 180) {
            x1 = 0;
            y1 = 0;
            x2 = 0;
            y2 = height;
            return true;
        }

        if (angle == 270) {
            x1 = width;
            y1 = 0;
            x2 = 0;
            y2 = 0;
            return true;
        }

        float slope = tan(convertFromDegToRad(90 - angle));

        float perpendicularSlope = -1 / slope;

        float halfHeight = height / 2;
        float halfWidth = width / 2;

        float cx, cy;

        if (angle < 90) {
            cx = halfWidth;
            cy = halfHeight;
        } else if (angle < 180) {
            cx = halfWidth;
            cy = -halfHeight;
        } else if (angle < 270) {
            cx = -halfWidth;
            cy = -halfHeight;
        } else {
            cx = -halfWidth;
            cy = halfHeight;
        }

        // Compute c (of y = mx + c) using the corner point.
        float c = cy - perpendicularSlope * cx;
        float ex = c / (slope - perpendicularSlope);
        float ey = perpendicularSlope * ex + c;

        x2 = halfWidth + ex;
        y2 = halfHeight - ey;

        x1 = halfWidth - ex;
        y1 = halfHeight + ey;
        return true;
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }
}

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
