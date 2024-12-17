/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "core/style/LengthUtil.h"
#include "core/style/CalcData.h"

namespace Starfish {

Optional<Length> LengthUtil::convertValueToLength(
    const CSSStyleValuePair& property)
{
    return LengthUtil::convertValueToLength(property.valueKind(),
                                            property.value());
}

Optional<Length> LengthUtil::convertValueToLength(
    CSSStyleValuePair::ValueKind kind, const CSSStyleValuePair::ValueData& data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Length();
    } else if (kind == CSSStyleValuePair::ValueKind::Length) {
        return data.m_length.toLength();
    } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
        return Length(Length::Percent, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Length(Length::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->calcValueType();
        if (type.isLength() || type.isPercentage() || type.isNumber()) {
            return Length(data.m_calc);
        } else {
            return Optional<Length>();
        }
    }
    return Optional<Length>();
}

Optional<Length> LengthUtil::backgroundPositionToLength(
    const CSSStyleValuePair& property)
{
    Length value;
    if (property.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        SideValue side = property.sideValue();
        if (side == SideValue::LeftSideValue) {
            value = Length(Length::Percent, 0.0f);
        } else if (side == SideValue::RightSideValue) {
            value = Length(Length::Percent, 1.0f);
        } else if (side == SideValue::TopSideValue) {
            value = Length(Length::Percent, 0.0f);
        } else if (side == SideValue::BottomSideValue) {
            value = Length(Length::Percent, 1.0f);
        } else if (side == SideValue::CenterSideValue) {
            value = Length(Length::Percent, 0.5f);
        } else {
            return Optional<Length>();
        }
    } else if (property.valueKind() == CSSStyleValuePair::ValueKind::Length) {
        value = property.lengthValue();
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::CalcValueKind) {
        value = property.lengthValue();
    } else if (property.valueKind() ==
               CSSStyleValuePair::ValueKind::Percentage) {
        value = Length(Length::Percent, property.percentageValue());
    } else {
        return Optional<Length>();
    }

    return value;
}

Optional<LengthSize> LengthUtil::backgroundSizeToLengthSize(
    const CSSStyleValuePair& property)
{
    LengthSize result;
    if (property.valueKind() == CSSStyleValuePair::ValueListKind) {
        ValueList* list = property.multiValue();
        if (list->size() >= 1) {
            Optional<Length> width =
                LengthUtil::convertValueToLength((*list)[0]);
            if (width.hasValue()) {
                result.m_width = width.getValue();
            }
        }
        if (list->size() >= 2) {
            Optional<Length> height =
                LengthUtil::convertValueToLength((*list)[1]);
            if (height.hasValue()) {
                result.m_height = height.getValue();
            }
        }
    } else {
        return Optional<LengthSize>();
    }

    return result;
}

} // namespace Starfish
