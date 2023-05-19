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

SVGTextElement::SVGTextElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_x(nullptr)
    , m_y(nullptr)
{
}

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
    } else if (ss->m_x == name) {
        if (x()->baseVal()->isUpdated()) {
            x()->baseVal()->unsetUpdated();
        } else {
            x()->baseVal()->updateListByAttribute();
        }
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    } else if (ss->m_y == name) {
        if (y()->baseVal()->isUpdated()) {
            y()->baseVal()->unsetUpdated();
        } else {
            y()->baseVal()->updateListByAttribute();
        }
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
    }
}

void SVGTextElement::updateAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x == name) {
        x()->baseVal()->updateAttributeByList();
    } else if (ss->m_y == name) {
        x()->baseVal()->updateAttributeByList();
    }
}

void SVGTextElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Nullable<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);

    StaticStrings* ss = starfish()->staticStrings();
    {
        auto attr = getAttributeOrVarReferencedValue(ss->m_fontDashFamily,
                                                     cssCustomValues);
        if (attr->length()) {
            CSSStyleDeclaration decl(this);
            auto str = attr->toUTF8NonGCString();
            decl.setPropertyInternal(CSSStyleValuePair::KeyKind::FontFamily,
                                     str.data(), str.length(), false);
            if (decl.cssValues().size()) {
                cssValues.push_back(decl.cssValues()[0]);
            }
        }
    }
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(fontDashSize, FontSize,
                                               cssCustomValues);
}

SVGAnimatedLengthList* SVGTextElement::x()
{
    if (m_x == nullptr) {
        SVGLengthList* xBaseVal =
            new SVGLengthList(this, starfish()->staticStrings()->m_x);
        SVGLengthList* xAnimVal =
            new SVGLengthList(this, starfish()->staticStrings()->m_x);

        m_x = new SVGAnimatedLengthList(document(), xBaseVal, xAnimVal);

        xBaseVal->updateListByAttribute();
        xAnimVal->updateListByAttribute();

        for (size_t i = 0; i < xAnimVal->length(); ++i) {
            xAnimVal->getItem(i)->setReadOnly();
        }
        xAnimVal->setReadOnly();
    }

    return m_x;
}

SVGAnimatedLengthList* SVGTextElement::y()
{
    if (m_y == nullptr) {
        SVGLengthList* yBaseVal =
            new SVGLengthList(this, starfish()->staticStrings()->m_y);
        SVGLengthList* yAnimVal =
            new SVGLengthList(this, starfish()->staticStrings()->m_y);

        m_y = new SVGAnimatedLengthList(document(), yBaseVal, yAnimVal);

        yBaseVal->updateListByAttribute();
        yAnimVal->updateListByAttribute();

        for (size_t i = 0; i < yAnimVal->length(); ++i) {
            yAnimVal->getItem(i)->setReadOnly();
        }
        yAnimVal->setReadOnly();
    }

    return m_y;
}

} // namespace Starfish
