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
#include "core/style/GradientData.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {
bool LinearGradientData::computeEndPoints(const int width, const int height,
                                          float& x1, float& y1, float& x2,
                                          float& y2)
{
    if (m_sc == 0) {
        float angle = fmodf(m_angleDeg, 360);
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

CSSGradientValue* LinearGradientData::convertToCSSGradientValue()
{
    CSSLinearGradientValue* gradient = new CSSLinearGradientValue();

    if (m_sc == 0) {
        gradient->setAngle(CSSAngle(m_angleDeg));
    } else {
        gradient->setSideOrConter(m_sc);
    }

    auto& cssColorStopList = gradient->cssColorStopList();

    for (auto item : m_colorStopList) {
        CSSColorStop* cs = new CSSColorStop();

        CSSStyleValuePair color;
        color.setColorValue(item->color());
        cs->setColor(color);

        if (item->offset().type().isPercentage()) {
            CSSStyleValuePair offset;
            offset.setPercentageValue(item->offset().percentageValue());
            cs->setOffset(offset);
        } else if (item->offset().type().isLength()) {
            CSSStyleValuePair offset =
                (CSSStyleDeclaration::lengthToCSSStyleValue(
                    item->offset().lengthValue()));
            cs->setOffset(offset);
        } else if (item->offset().type().isNone()) {
            CSSStyleValuePair offset;
            cs->setOffset(offset);
        }

        cssColorStopList.push_back(cs);
    }

    return gradient;
}
}
