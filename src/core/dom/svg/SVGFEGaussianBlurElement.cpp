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
#include "Starfish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGFEGaussianBlurElement.h"
#include "core/dom/DOMTokenList.h"

namespace Starfish {
SVGFEGaussianBlurElement::SVGFEGaussianBlurElement(Document* document,
                                                   const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEGaussianBlurElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEGaussianBlurElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEGaussianBlurElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_in1));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_stdDeviationX));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_stdDeviationY));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEGaussianBlurElement, m_edgeMode));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEGaussianBlurElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEGaussianBlurElement::didAttributeChanged(QualifiedName name,
                                                   Optional<String*> old,
                                                   String* value,
                                                   bool attributeCreated,
                                                   bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    Optional<SVGFilterElement*> filterElement;
    if (parentElement() && parentElement()->isSVGFilterElement()) {
        filterElement = parentElement()->asSVGFilterElement();
    }

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in1 == name) {
        if (filterElement.hasValue()) {
            filterElement->attributeOfPaintServerLikeUpdated();
        }
        in1()->setBaseVal(value);
    } else if (ss->m_stdDeviation == name) {
        if (filterElement.hasValue()) {
            filterElement->attributeOfPaintServerLikeUpdated();
        }
        GCVector<StringView> tokens;
        DOMTokenList::tokenize(value, tokens);
        if (tokens.size() == 1) {
            stdDeviationX()->setBaseVal(String::parseFloat(&tokens[0]));
            stdDeviationY()->setBaseVal(String::parseFloat(&tokens[0]));
        } else if (tokens.size() > 1) {
            stdDeviationX()->setBaseVal(String::parseFloat(&tokens[0]));
            stdDeviationY()->setBaseVal(String::parseFloat(&tokens[1]));
        }
    } else if (ss->m_stdDeviationX == name) {
        if (filterElement.hasValue()) {
            filterElement->attributeOfPaintServerLikeUpdated();
        }
        stdDeviationX()->setBaseVal(String::parseFloat(value));
    } else if (ss->m_stdDeviationY == name) {
        if (filterElement.hasValue()) {
            filterElement->attributeOfPaintServerLikeUpdated();
        }
        stdDeviationY()->setBaseVal(String::parseFloat(value));
    } else if (ss->m_edgeMode == name) {
        if (filterElement.hasValue()) {
            filterElement->attributeOfPaintServerLikeUpdated();
        }
    }
}

void SVGFEGaussianBlurElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in1 == name) {
        setAttribute(ss->m_in1, in1()->baseVal());
    } else if (ss->m_stdDeviation == name) {
        setAttribute(ss->m_stdDeviationX,
                     String::fromFloat(stdDeviationX()->baseVal()));
        setAttribute(ss->m_stdDeviationY,
                     String::fromFloat(stdDeviationY()->baseVal()));
    } else if (ss->m_stdDeviationX == name) {
        setAttribute(ss->m_stdDeviationX,
                     String::fromFloat(stdDeviationX()->baseVal()));
    } else if (ss->m_stdDeviationY == name) {
        setAttribute(ss->m_stdDeviationY,
                     String::fromFloat(stdDeviationY()->baseVal()));
    } else if (ss->m_edgeMode == name) {
        m_edgeMode->updateAttribute();
    }
}

void SVGFEGaussianBlurElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

SVGAnimatedString* SVGFEGaussianBlurElement::in1()
{
    if (!m_in1.hasValue()) {
        // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/in
        m_in1 = new SVGAnimatedString(
            document(), String::createASCIIString("SourceGraphic"),
            String::emptyString);
    }
    return m_in1.getValue();
}

SVGAnimatedNumber* SVGFEGaussianBlurElement::stdDeviationX()
{
    if (!m_stdDeviationX.hasValue()) {
        m_stdDeviationX = new SVGAnimatedNumber(document(), 0, 0);
    }
    return m_stdDeviationX.getValue();
}

SVGAnimatedNumber* SVGFEGaussianBlurElement::stdDeviationY()
{
    if (!m_stdDeviationY.hasValue()) {
        m_stdDeviationY = new SVGAnimatedNumber(document(), 0, 0);
    }
    return m_stdDeviationY.getValue();
}

SVGAnimatedEnumeration* SVGFEGaussianBlurElement::edgeMode()
{
    if (!m_edgeMode.hasValue()) {
        m_edgeMode =
            new SVGAnimatedEnumeration(this, staticStrings()->m_edgeMode, 0, 0);
    }
    return m_edgeMode.getValue();
}

void SVGFEGaussianBlurElement::setStdDeviation(float stdDeviationX,
                                               float stdDeviationY)
{
    if (!m_stdDeviationX.hasValue()) {
        m_stdDeviationX = new SVGAnimatedNumber(document(), 0, 0);
    }
    m_stdDeviationX->setBaseVal(stdDeviationX);

    if (!m_stdDeviationY.hasValue()) {
        m_stdDeviationY = new SVGAnimatedNumber(document(), 0, 0);
    }
    m_stdDeviationY->setBaseVal(stdDeviationY);
}
} // namespace Starfish
