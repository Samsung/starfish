/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/svg/SVGSVGElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

SVGSVGElement::SVGSVGElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_hasViewBox(false)
    , m_viewBox(0, 0, 0, 0)
{
    STARFISH_ASSERT(document != nullptr);
}

void* SVGSVGElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGSVGElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGSVGElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGSVGElement, m_x));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGSVGElement, m_y));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGSVGElement, m_width));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGSVGElement, m_height));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGSVGElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGSVGElement::didAttributeChanged(QualifiedName name,
                                        Optional<String*> old, String* value,
                                        bool attributeCreated,
                                        bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    if (name == starfish()->staticStrings()->m_viewBox) {
        m_hasViewBox = false;
        auto utf8Str = value->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, utf8Str.data(),
                                              utf8Str.length(), ",", 1);
        if (tokens.size() == 4) {
            float x = 0, y = 0, w = 0, h = 0;
            if (CSSPropertyParser::parseNumber(tokens[0].data(), 1 << 0, &x) &&
                CSSPropertyParser::parseNumber(tokens[1].data(), 1 << 0, &y) &&
                CSSPropertyParser::parseNumber(tokens[2].data(), 0, &w) &&
                CSSPropertyParser::parseNumber(tokens[3].data(), 0, &h)) {
                m_viewBox = Unit::Rect(x, y, w, h);
                m_hasViewBox = true;
            }
        }

        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsLayout();
    }
}

void SVGSVGElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x == name) {
        setAttribute(ss->m_x, x()->baseVal()->valueAsString());
    } else if (ss->m_y == name) {
        setAttribute(ss->m_y, y()->baseVal()->valueAsString());
    } else if (ss->m_width == name) {
        setAttribute(ss->m_width, width()->baseVal()->valueAsString());
    } else if (ss->m_height == name) {
        setAttribute(ss->m_height, height()->baseVal()->valueAsString());
    }
}

void SVGSVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);

    if (hasAttribute(starfish()->staticStrings()->m_width) == SIZE_MAX &&
        hasAttribute(starfish()->staticStrings()->m_height) == SIZE_MAX) {
        if (m_hasViewBox) {
            float w, h;
            if (m_viewBox.width() / m_viewBox.height() > 1) {
                w = 1;
                h = m_viewBox.height() / m_viewBox.width();
            } else {
                w = m_viewBox.width() / m_viewBox.height();
                h = 1;
            }

            CSSStyleValuePair pair;
            pair.setKeyKind(CSSStyleValuePair::Width);
            pair.setValueKind(CSSStyleValuePair::Percentage);
            pair.setPercentageValue(w);
            cssValues.push_back(pair);
            pair.setKeyKind(CSSStyleValuePair::Height);
            pair.setPercentageValue(h);
            cssValues.push_back(pair);
        }
    }
}

NativeImageData::PreserveAspectRatioAlign
SVGSVGElement::preserveAspectRatioAlign()
{
    if (hasAttribute(starfish()->staticStrings()->m_preserveAspectRatio) ==
        SIZE_MAX) {
        bool hasViewbox =
            hasAttribute(starfish()->staticStrings()->m_viewBox) != SIZE_MAX;
        return hasViewbox ? NativeImageData::xMidYMid : NativeImageData::None;
    }
    return m_preserveAspectRatioAlign;
}

NativeImageData::PreserveAspectRatioMeetOrSlice
SVGSVGElement::preserveAspectRatioMeetOrSlice()
{
    return m_preserveAspectRatioMeetOrSlice;
}

SVGNumber* SVGSVGElement::createSVGNumber()
{
    return new SVGNumber(this, AtomicString::emptyAtomicString());
}

SVGLength* SVGSVGElement::createSVGLength()
{
    return new SVGLength(this, AtomicString::emptyAtomicString());
}

SVGAngle* SVGSVGElement::createSVGAngle()
{
    return new SVGAngle(this, AtomicString::emptyAtomicString());
}

SVGTransform* SVGSVGElement::createSVGTransform()
{
    return new SVGTransform(this, AtomicString::emptyAtomicString());
}

void SVGSVGElement::pauseAnimations()
{
    STARFISH_UNSUPPORTED_METHOD();
}

void SVGSVGElement::unpauseAnimations()
{
    STARFISH_UNSUPPORTED_METHOD();
}

} // namespace Starfish
