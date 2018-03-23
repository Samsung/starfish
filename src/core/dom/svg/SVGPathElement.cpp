/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGPathElement.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

QualifiedName SVGPathElement::name()
{
    return starFish()->staticStrings()->m_svgpathTagName;
}

void SVGPathElement::didAttributeChanged(QualifiedName name, String* old,
                                         String* value, bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();

    if (ss->m_d == name) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        setNeedsPainting();
    }
}

void SVGPathElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);

    String* d = getAttributeOrEmpty(starFish()->staticStrings()->m_d);
    CSSStyleDeclaration decl(this);
    auto buf = d->toUTF8NonGCString();
    decl.setD(buf.data(), buf.length(), false);
    if (decl.hasCSSValuePair(CSSStyleValuePair::KeyKind::D)) {
        cssValues.push_back(decl.cssValues()[0]);
    } else if (d->length()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
        pair.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        pair.setStringValue(d);
        cssValues.push_back(pair);
    }
}
}
