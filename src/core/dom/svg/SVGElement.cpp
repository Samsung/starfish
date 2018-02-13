/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/svg/SVGElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

void* SVGElement::operator new(size_t size)
{
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
            setNeedsLayout();
        } else if (ss->m_y == name) {
            setNeedsLayout();
        } else if (ss->m_width == name) {
            setNeedsLayout();
        } else if (ss->m_height == name) {
            setNeedsLayout();
        }
    }

    if (needsFillAttributes()) {
        if (ss->m_fill == name) {
            setNeedsPainting();
        } else if (ss->m_fillOpacity == name) {
            setNeedsPainting();
        } else if (ss->m_fillRule == name) {
            setNeedsPainting();
        }
    }

    if (needsStrokeAttributes()) {
        if (ss->m_stroke == name) {
            setNeedsPainting();
        } else if (ss->m_strokeWidth == name) {
            setNeedsPainting();
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

    String* width = getAttributeOrEmpty(starFish()->staticStrings()->m_width);
    if (width->length()) {
        pair.setKeyKind(CSSStyleValuePair::Width);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Length);
        auto s = width->toUTF8NonGCString();
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            cssValues.push_back(pair);
        }
    }

    String* height = getAttributeOrEmpty(starFish()->staticStrings()->m_height);
    if (height->length()) {
        pair.setKeyKind(CSSStyleValuePair::Height);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Length);
        auto s = height->toUTF8NonGCString();
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            cssValues.push_back(pair);
        }
    }

    if (needsFillAttributes()) {
        String* fill = getAttributeOrEmpty(starFish()->staticStrings()->m_fill);
        if (fill->length()) {
            pair.setKeyKind(CSSStyleValuePair::Fill);

            auto fillStr = fill->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, fillStr.data(),
                                                  fillStr.length());
            if (pair.updateValueFill(tokens)) {
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
            if (pair.updateValueFillRule(tokens)) {
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
            if (pair.updateValueFillOpacity(tokens)) {
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
            if (pair.updateValueStroke(tokens)) {
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
            if (pair.updateValueStrokeWidth(tokens)) {
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

int SVGElement::tabIndex() const
{
    if (supportsFocus()) {
        return Element::tabIndex();
    }
    return -1;
}

void* SVGNamedElement::operator new(size_t size)
{
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
