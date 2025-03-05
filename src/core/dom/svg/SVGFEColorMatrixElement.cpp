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
#include "core/dom/svg/SVGFEColorMatrixElement.h"
#include "core/dom/svg/SVGAnimatedNumberList.h"
#include "core/dom/DOMTokenList.h"

namespace Starfish {
SVGFEColorMatrixElement::SVGFEColorMatrixElement(Document* document,
                                                 const QualifiedName& qname)
    : SVGFilterPrimitiveStandardAttributes(document, qname)
{
}

void* SVGFEColorMatrixElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGFEColorMatrixElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGFEColorMatrixElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEColorMatrixElement, m_in1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEColorMatrixElement, m_type));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFEColorMatrixElement, m_values));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGFEColorMatrixElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGFEColorMatrixElement::didAttributeChanged(QualifiedName name,
                                                  Optional<String*> old,
                                                  String* value,
                                                  bool attributeCreated,
                                                  bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in1 == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        in1()->setBaseVal(value);
    } else if (ss->m_type == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        if (type()->isUpdated() == false) {
            if (value->equals("matrix")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    MatrixTypes::SVG_FECOLORMATRIX_TYPE_MATRIX);
            } else if (value->equals("saturate")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    MatrixTypes::SVG_FECOLORMATRIX_TYPE_SATURATE);
            } else if (value->equals("hueRotate")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    MatrixTypes::SVG_FECOLORMATRIX_TYPE_HUEROTATE);
            } else if (value->equals("luminanceToAlpha")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    MatrixTypes::SVG_FECOLORMATRIX_TYPE_LUMINANCETOALPHA);
            } else {
                m_type->setBaseValWithoutUpdateAttribute(
                    MatrixTypes::SVG_FECOLORMATRIX_TYPE_UNKNOWN);
            }
        }
    } else if (ss->m_values == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        SVGNumberList* valueList = values()->baseVal();
        GCVector<StringView> v;
        StringUtils::wordTokenizer(value, v);
        for (size_t i = 0; i < v.size(); i++) {
            valueList->appendItem(
                new SVGNumber(this, AtomicString::emptyAtomicString(),
                              String::parseFloat(v[i].substring())));
        }
    }
}

void SVGFEColorMatrixElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_in1 == name) {
        setAttribute(ss->m_in1, in1()->baseVal());
    } else if (ss->m_type == name) {
        switch (type()->baseVal()) {
        case MatrixTypes::SVG_FECOLORMATRIX_TYPE_UNKNOWN:
            setAttribute(ss->m_type, String::emptyString);
            break;
        case MatrixTypes::SVG_FECOLORMATRIX_TYPE_MATRIX:
            setAttribute(ss->m_type, String::fromUTF8("matrix"));
            break;
        case MatrixTypes::SVG_FECOLORMATRIX_TYPE_SATURATE:
            setAttribute(ss->m_type, String::fromUTF8("saturate"));
            break;
        case MatrixTypes::SVG_FECOLORMATRIX_TYPE_HUEROTATE:
            setAttribute(ss->m_type, String::fromUTF8("hueRotate"));
            break;
        case MatrixTypes::SVG_FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
            setAttribute(ss->m_type, String::fromUTF8("luminanceToAlpha"));
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
        }
    } else if (ss->m_values == name) {
        setAttribute(ss->m_values, values()->baseVal()->toString());
    }
}

void SVGFEColorMatrixElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

SVGAnimatedString* SVGFEColorMatrixElement::in1()
{
    if (!m_in1.hasValue()) {
        // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/in
        m_in1 = new SVGAnimatedString(
            document(), String::createASCIIString("SourceGraphic"),
            String::emptyString);
    }
    return m_in1.getValue();
}

SVGAnimatedEnumeration* SVGFEColorMatrixElement::type()
{
    if (!m_type.hasValue()) {
        m_type = new SVGAnimatedEnumeration(
            this, staticStrings()->m_type,
            MatrixTypes::SVG_FECOLORMATRIX_TYPE_MATRIX,
            MatrixTypes::SVG_FECOLORMATRIX_TYPE_MATRIX);
    }
    return m_type.getValue();
}

SVGAnimatedNumberList* SVGFEColorMatrixElement::values()
{
    if (!m_values.hasValue()) {
        m_values = new SVGAnimatedNumberList(
            document(),
            new SVGNumberList(this, AtomicString::emptyAtomicString()),
            new SVGNumberList(this, AtomicString::emptyAtomicString()));
    }
    return m_values.getValue();
}
} // namespace Starfish
