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
#include "core/style/CSSGradientValue.h"
#include "core/style/GradientData.h"

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

    for (auto cs : m_cssColorStopList) {
        result.appendString(", ");
        result.appendString(cs->color().toString());

        if (cs->offset().valueKind() != CSSStyleValuePair::ValueKind::None) {
            result.appendChar(' ');
            result.appendString(cs->offset().toString());
        }
    }

    result.appendChar(')');

    return result.finalize();
}

GradientData* CSSLinearGradientValue::convertToGradientData()
{
    LinearGradientData* gradient = new LinearGradientData();

    if (m_sc == 0) {
        gradient->setAngle(m_angle.toDegreeValue());
    } else {
        gradient->setSideOrConter(m_sc);
    }

    auto& colorStopList = gradient->colorStopList();

    for (auto item : m_cssColorStopList) {
        ColorStop* cs = new ColorStop();
        auto color = item->color();
        if (color.valueKind() ==
            CSSStyleValuePair::ValueKind::NamedColorValueKind) {
            Unit::Color c =
                NamedColor::namedColorToColor(color.namedColorValue());
            cs->setColor(c);
        } else {
            Unit::Color c = color.colorValue();
            cs->setColor(c);
        }

        auto offset = item->offset();
        if (offset.valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
            cs->setOffset(ColorStopOffsetValue(offset.percentageValue()));
        } else if (offset.valueKind() == CSSStyleValuePair::ValueKind::Length) {
            cs->setOffset(ColorStopOffsetValue(offset.lengthValue()));
        } else if (offset.valueKind() == CSSStyleValuePair::ValueKind::None) {
            cs->setOffset(ColorStopOffsetValue());
        }

        colorStopList.push_back(cs);
    }

    return gradient;
}

} /* namespace StarFish */
