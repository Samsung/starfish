/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#include "core/style/StyleTransformOrigin.h"
#include "core/style/LengthUtil.h"

namespace Starfish {

void StyleTransformOrigin::initializeOriginValue(
    const CSSStyleValuePair& cssValuePair)
{
    // TODO: Handle Inherit?

    if ((cssValuePair.valueKind() == CSSStyleValuePair::ValueKind::Initial) ||
        (cssValuePair.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
        setOriginValue(Length(Length::Percent, 0.5f),
                       Length(Length::Percent, 0.5f),
                       Length(Length::Fixed, 0.f));
    } else if (cssValuePair.valueKind() ==
               CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = cssValuePair.multiValue();
        Length xAxis, yAxis, zAxis;

        xAxis = Length(Length::Percent, 0.5f);
        yAxis = Length(Length::Percent, 0.5f);
        zAxis = Length(Length::Fixed, 0.f);

        for (unsigned int i = 0; i < std::min(list->size(), (size_t)2); i++) {
            const CSSStyleValuePair& item = (*list)[i];
            if (item.valueKind() ==
                CSSStyleValuePair::ValueKind::SideValueKind) {
                if (item.sideValue() == SideValue::LeftSideValue) {
                    xAxis = Length(Length::Percent, 0.0f);
                } else if (item.sideValue() == SideValue::RightSideValue) {
                    xAxis = Length(Length::Percent, 1.0f);
                } else if (item.sideValue() == SideValue::CenterSideValue) {
                } else if (item.sideValue() == SideValue::TopSideValue) {
                    yAxis = Length(Length::Percent, 0.0f);
                } else if (item.sideValue() == SideValue::BottomSideValue) {
                    yAxis = Length(Length::Percent, 1.0f);
                }
            } else {
                if (i == 0) {
                    Optional<Length> nXAxis = LengthUtil::convertValueToLength(
                        item.valueKind(), item.value());
                    if (nXAxis.hasValue()) {
                        xAxis = nXAxis.getValue();
                    }
                } else {
                    Optional<Length> nYAxis = LengthUtil::convertValueToLength(
                        item.valueKind(), item.value());
                    if (nYAxis.hasValue()) {
                        yAxis = nYAxis.getValue();
                    }
                }
            }
        }

        if (list->size() == 3) {
            Optional<Length> nZAxis = LengthUtil::convertValueToLength(
                (*list)[2].valueKind(), (*list)[2].value());
            if (nZAxis.hasValue()) {
                zAxis = nZAxis.getValue();
            }
        }

        setOriginValue(xAxis, yAxis, zAxis);
    } else {
        STARFISH_UNIMPLEMENTED();
    }
}

} // namespace Starfish
