/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "core/style/CSSParser.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGEllipseElement.h"

namespace Starfish {

void SVGEllipseElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    STARFISH_ASSERT(old != nullptr);
    STARFISH_ASSERT(value != nullptr);

    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_cx == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_cy == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
    if (ss->m_rx == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_ry == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
}

void SVGEllipseElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cx, CX);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cy, CY);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(rx, RX);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(ry, RY);
}
}
