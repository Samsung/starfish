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
#include "core/dom/svg/SVGElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

void* SVGElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGElement)] = { 0 };
        Element::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGElement::didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved)
{
    Element::didAttributeChanged(name, old, value, attributeCreated,
                                 attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();

    if (needsGeometryAttributes()) {
        if (ss->m_x == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsLayout();
        } else if (ss->m_y == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsLayout();
        }
    }

    if (needsSizingAttributes()) {
        if (ss->m_width == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsLayout();
        } else if (ss->m_height == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsLayout();
        }
    }

    if (needsFillAttributes()) {
        if (ss->m_fill == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsPainting();
        } else if (ss->m_fillOpacity == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsPainting();
        } else if (ss->m_fillRule == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsPainting();
        }
    }

    if (needsStrokeAttributes()) {
        if (ss->m_stroke == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsPainting();
        } else if (ss->m_strokeWidth == name) {
            setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
            setNeedsPainting();
        }
    }

    if (needsPreserveAspectRatioValue()) {
        if (name == starFish()->staticStrings()->m_preserveAspectRatio) {
#define SET_PARV(name)                                      \
    else if (value->equals(#name))                          \
    {                                                       \
        m_preserveAspectRatioValue = NativeImageData::name; \
    }

            if (value->equals("none")) {
                m_preserveAspectRatioValue = NativeImageData::None;
            }
            SET_PARV(xMinYMin)
            SET_PARV(xMidYMin)
            SET_PARV(xMaxYMin)
            SET_PARV(xMinYMid)
            SET_PARV(xMidYMid)
            SET_PARV(xMaxYMid)
            SET_PARV(xMinYMax)
            SET_PARV(xMidYMax)
            SET_PARV(xMaxYMax)
            else
            {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
#undef SET_PARV
        }
    }
}

String* SVGElement::xmlbase()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_xmlBase);
}

void SVGElement::setXmlbase(String* str)
{
    setAttribute(starFish()->staticStrings()->m_xmlBase, str);
}

SVGElement* SVGElement::ownerSVGElement()
{
    // The nearest ancestor ‘svg’ element. Null if the given element is the
    // outermost svg element.
    Element* e = parentElement();

    while (!e->isSVGSVGElement()) {
        e = e->parentElement();
    }

    return (SVGElement*)e;
}

SVGElement* SVGElement::viewportElement()
{
    // The element which established the current viewport. Often, the nearest
    // ancestor ‘svg’ element. Null if the given element is the outermost svg
    // element.
    return ownerSVGElement();
}

void SVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    Element::styleForPresentationAttribute(cssValues);
    CSSStyleValuePair pair;

    if (needsGeometryAttributes()) {
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x, X);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y, Y);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(width, Width);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(height, Height);
    }

    if (needsFillAttributes()) {
        String* fill = getAttributeOrEmpty(starFish()->staticStrings()->m_fill);
        if (fill->length()) {
            pair.setKeyKind(CSSStyleValuePair::Fill);

            auto fillStr = fill->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, fillStr.data(),
                                                  fillStr.length());
            if (pair.updateValueFill(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* fillRule =
            getAttributeOrEmpty(starFish()->staticStrings()->m_fillRule);
        if (fillRule->length()) {
            pair.setKeyKind(CSSStyleValuePair::FillRule);

            auto str = fillRule->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueFillRule(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* fillOpacity =
            getAttributeOrEmpty(starFish()->staticStrings()->m_fillOpacity);
        if (fillOpacity->length()) {
            pair.setKeyKind(CSSStyleValuePair::FillOpacity);

            auto str = fillOpacity->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueFillOpacity(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (needsStrokeAttributes()) {
        String* stroke =
            getAttributeOrEmpty(starFish()->staticStrings()->m_stroke);
        if (stroke->length()) {
            pair.setKeyKind(CSSStyleValuePair::Stroke);

            auto str = stroke->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueStroke(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* strokeWidth =
            getAttributeOrEmpty(starFish()->staticStrings()->m_strokeWidth);
        if (strokeWidth->length()) {
            pair.setKeyKind(CSSStyleValuePair::StrokeWidth);

            auto str = strokeWidth->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueStrokeWidth(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (needsTransformAttributes()) {
        String* transform =
            getAttributeOrEmpty(starFish()->staticStrings()->m_transform);
        if (transform->length()) {
            pair.setKeyKind(CSSStyleValuePair::Transform);

            auto str = transform->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueTransform(tokens, true)) {
                cssValues.push_back(pair);
            }
        }
    }
}

int SVGElement::tabIndex()
{
    if (supportsFocus()) {
        return Element::tabIndex();
    }
    return -1;
}

void* SVGNamedElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGNamedElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGNamedElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGNamedElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
}
