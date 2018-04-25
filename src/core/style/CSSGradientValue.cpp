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

void CSSGradientValue::convertCSSColorStopsToColorStops(
    GCVector<ColorStop*>& out)
{
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
        if (offset.valueKind() != CSSStyleValuePair::ValueKind::None) {
            cs->setOffset(offset.toLengthValue());
        }

        out.push_back(cs);
    }
}

String* CSSGradientValue::colorStopListToString()
{
    StringBuilder result;
    for (size_t i = 0; i < m_cssColorStopList.size(); ++i) {
        auto* cs = m_cssColorStopList[i];
        if (i > 0) {
            result.appendString(", ");
        }
        result.appendString(cs->color().toString());
        if (cs->offset().valueKind() != CSSStyleValuePair::ValueKind::None) {
            result.appendChar(' ');
            result.appendString(cs->offset().toString());
        }
    }
    return result.finalize();
}

String* CSSLinearGradientValue::toString()
{
    StringBuilder result;

    result.appendString("linear-gradient(");
    CSSLinearGradientValue* linear = (CSSLinearGradientValue*)this;
    if (linear->angle().toDegreeValue() != 180) {
        result.appendString(linear->angle().toString());
        result.appendString(", ");
    } else if (m_sc) {
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
        result.appendString(", ");
    }

    result.appendString(colorStopListToString());
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

    convertCSSColorStopsToColorStops(gradient->colorStopList());

    return gradient;
}

String* CSSRadialGradientValue::toString()
{
    StringBuilder result;

    result.appendString("radial-gradient(");

    if (m_shape == RadialGradientShape::Circle) {
        result.appendString("circle ");
    } else if (m_shape == RadialGradientShape::Elipse) {
        result.appendString("elipse ");
    }

    if (m_size.hasKeyword()) {
        if (m_size.keyword() == RadialGradientSizeKeyword::ClosetSide) {
            result.appendString("closest-side ");
        } else if (m_size.keyword() ==
                   RadialGradientSizeKeyword::FarthestSide) {
            result.appendString("farthest-side ");
        } else if (m_size.keyword() ==
                   RadialGradientSizeKeyword::ClosetCorner) {
            result.appendString("closest-corner ");
        } else if (m_size.keyword() ==
                   RadialGradientSizeKeyword::FarthestCorner) {
            result.appendString("farthest-corner ");
        }
    } else if (m_size.hasFirstRadius()) {
        result.appendString(m_size.firstRadius().toString());
        result.appendChar(' ');
        if (m_size.hasSecondRadius()) {
            result.appendString(m_size.secondRadius().toString());
            result.appendChar(' ');
        }
    }
    if (m_positionX.valueKind() ==
        CSSStyleValuePair::ValueKind::ValueListKind) {
        result.appendString("at ");
        result.appendString(m_positionX.toString());
        result.appendChar(' ');
        result.appendString(m_positionY.toString());
        result.appendString(", ");
    }

    result.appendString(colorStopListToString());
    result.appendChar(')');

    return result.finalize();
}

GradientData* CSSRadialGradientValue::convertToGradientData()
{
    RadialGradientData* gradient = new RadialGradientData();

    // Position of gradient center
    if (m_positionX.valueKind() ==
        CSSStyleValuePair::ValueKind::ValueListKind) {
        auto& x = m_positionX.multiValue()->front();
        if (x.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
            gradient->setHorizontalSide(x.sideValue());
        } else if (x.valueKind() != CSSStyleValuePair::ValueKind::None) {
            gradient->setHorizontalSideOffset(x.toLengthValue());
        }
    }
    if (m_positionY.valueKind() ==
        CSSStyleValuePair::ValueKind::ValueListKind) {
        auto& y = m_positionY.multiValue()->front();
        if (y.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
            gradient->setVerticalSide(y.sideValue());
        } else if (y.valueKind() != CSSStyleValuePair::ValueKind::None) {
            gradient->setVerticalSideOffset(y.toLengthValue());
        }
    }

    // Shape
    gradient->setShape(m_shape);

    // Size of the gradient's ending shape
    gradient->setKeyword(m_size.keyword());
    if (m_size.hasFirstRadius()) {
        gradient->setFirstRadius(m_size.firstRadius().toLengthValue());
    }
    if (m_size.hasSecondRadius()) {
        gradient->setSecondRadius(m_size.secondRadius().toLengthValue());
    }

    convertCSSColorStopsToColorStops(gradient->colorStopList());
    return gradient;
}
} /* namespace StarFish */
