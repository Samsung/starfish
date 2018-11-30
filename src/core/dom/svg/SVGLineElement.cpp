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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGLineElement.h"

namespace Starfish {

void SVGLineElement::didAttributeChanged(QualifiedName name, String* old,
                                         String* value, bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_x1 == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    } else if (ss->m_x2 == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    } else if (ss->m_y1 == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    } else if (ss->m_y2 == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    }
}

void SVGLineElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);
}
}
