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
#include "core/dom/svg/SVGSymbolElement.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

SVGSymbolElement::SVGSymbolElement(Document* document,
                                   const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_hasViewBox(false)
    , m_viewBox(0, 0, 0, 0)
{
}

void* SVGSymbolElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGSymbolElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGSymbolElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGSymbolElement, m_x));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGSymbolElement, m_y));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGSymbolElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGSymbolElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGElement::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (name == starfish()->staticStrings()->m_viewBox) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsLayout();
    }
}

void SVGSymbolElement::didAttributeChanged(QualifiedName name,
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
            if (CSSPropertyParser::parseNumber(
                    tokens[0].data(), tokens[0].length(), 1 << 0, &x) &&
                CSSPropertyParser::parseNumber(
                    tokens[1].data(), tokens[1].length(), 1 << 0, &y) &&
                CSSPropertyParser::parseNumber(tokens[2].data(),
                                               tokens[2].length(), 0, &w) &&
                CSSPropertyParser::parseNumber(tokens[3].data(),
                                               tokens[3].length(), 0, &h)) {
                m_viewBox = Unit::Rect(x, y, w, h);
                m_hasViewBox = true;
            }
        }
    }
}

NativeImageData::PreserveAspectRatioAlign
SVGSymbolElement::preserveAspectRatioAlign()
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
SVGSymbolElement::preserveAspectRatioMeetOrSlice()
{
    return m_preserveAspectRatioMeetOrSlice;
}

} // namespace Starfish
