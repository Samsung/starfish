/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/svg/SVGTextElement.h"

namespace Starfish {

void SVGTextElement::didAttributeChanged(QualifiedName name, String* old,
                                         String* value, bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_fontDashSize == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_fontDashFamily == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_textAnchor == name) {
        TextAnchor ta = TextAnchor::START;
        if (value->equals(String::createASCIIString("middle"))) {
            ta = TextAnchor::MIDDLE;
        } else if (value->equals(String::createASCIIString("end"))) {
            ta = TextAnchor::END;
        }
        setTextAnchor(ta);
        setNeedsPainting();
    } else if (ss->m_alignmentBaseline == name) {
        AlignmentBaseline al = AlignmentBaseline::AUTO;
        if (value->equals(String::createASCIIString("middle"))) {
            al = AlignmentBaseline::MIDDLE;
        }
        setAlignmentBaseline(al);
        setNeedsPainting();
    }
}

void SVGTextElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);

    StaticStrings* ss = starfish()->staticStrings();
    {
        auto attr = getAttribute(ss->m_fontDashFamily);
        if (attr.hasValue()) {
            CSSStyleDeclaration decl(this);
            auto str = attr.getValue()->toUTF8NonGCString();
            decl.setFontFamily(str.data(), str.length(), false);
            if (decl.cssValues().size()) {
                cssValues.push_back(decl.cssValues()[0]);
            }
        }
    }
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(fontDashSize, FontSize);
}
}
