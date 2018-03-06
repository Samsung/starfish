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
#include "core/dom/HTMLFontElement.h"

namespace StarFish {

void* HTMLFontElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLFontElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLFontElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLFontElement::name()
{
    return starFish()->staticStrings()->m_fontTagName;
}

void HTMLFontElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_color) {
        if (attributeCreated) {
            m_hasColorAttribute = true;
        }
        if (attributeRemoved) {
            m_hasColorAttribute = false;
        }
        if (!old->equals(value)) {
            setAttribute(starFish()->staticStrings()->m_color, value);
        }
        setNeedsStyleRecalc();
    } else if (name == starFish()->staticStrings()->m_size) {
        if (attributeCreated) {
            m_hasSizeAttribute = true;
        }
        if (attributeRemoved) {
            m_hasSizeAttribute = false;
        }
        if (!old->equals(value)) {
            setAttribute(starFish()->staticStrings()->m_size, value);
        }
        setNeedsStyleRecalc();
    }
}

// https://html.spec.whatwg.org/multipage/rendering.html#rendering
static bool parseLegacyFontSize(String* size, int& fontSize)
{
    if (size->isEmpty()) {
        return false;
    }

    // Step 1 ~ 4
    auto s = size->toUTF8NonGCString();
    const char* position = s.data();
    const char* end = position + size->length();
    while (position < end) {
        if (!String::isASCIISpace(*position)) {
            break;
        }
        position++;
    }

    if (position == end) {
        return false;
    }

    // Step 5
    enum { RelativePlus, RelativeMinus, Absolute } mode;
    switch (*position) {
    case '+':
        mode = RelativePlus;
        ++position;
        break;
    case '-':
        mode = RelativeMinus;
        ++position;
        break;
    default:
        mode = Absolute;
        break;
    }

    // Step 6 ~ 7
    StringBuilder digits;
    while (position < end) {
        if (!String::isASCIIDigit(*position)) {
            break;
        }
        digits.appendChar(*position++);
    }

    if (!digits.contentLength()) {
        return false;
    }

    // Step 8 ~ 11
    int value = String::parseInt(digits.finalize());

    if (mode == RelativePlus) {
        value += 3;
    } else if (mode == RelativeMinus) {
        value = 3 - value;
    }

    if (value > 7) {
        value = 7;
    }
    if (value < 1) {
        value = 1;
    }

    fontSize = value;
    return true;
}

void HTMLFontElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    if (m_hasColorAttribute) {
        CSSStyleValuePair pair;
        String* color =
            getAttributeOrEmpty(starFish()->staticStrings()->m_color);
        auto utf8Str = color->toNullableUTF8String();
        // TODO: Some obsolete legacy attributes parse colors in a more
        // complicated manner, using the rules for parsing a legacy color value.
        // https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#rules-for-parsing-a-legacy-colour-value
        if (pair.updateValueUnitColor(utf8Str.m_buffer)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Color);
            cssValues.push_back(pair);
        }
    }

    if (m_hasSizeAttribute) {
        String* size = getAttributeOrEmpty(starFish()->staticStrings()->m_size);
        int fontSize = 0;
        if (!parseLegacyFontSize(size, fontSize)) {
            return;
        }

        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::FontSize);
        pair.setValueKind(CSSStyleValuePair::ValueKind::FontSizeValueKind);

        // https://html.spec.whatwg.org/multipage/rendering.html#rendering
        // Step 12
        switch (fontSize) {
        case 1:
            pair.setValue(FontSizeValue::XSmallFontSizeValue);
            break;
        case 2:
            pair.setValue(FontSizeValue::SmallFontSizeValue);
            break;
        case 3:
            pair.setValue(FontSizeValue::MediumFontSizeValue);
            break;
        case 4:
            pair.setValue(FontSizeValue::LargeFontSizeValue);
            break;
        case 5:
            pair.setValue(FontSizeValue::XLargeFontSizeValue);
            break;
        case 6:
            pair.setValue(FontSizeValue::XXLargeFontSizeValue);
            break;
        case 7:
            // The 'xxx-large' value is a non-CSS value.
            // This value indicates a font size 50% larger than 'xx-large'.
            pair.setValue(FontSizeValue::XXXLargeFontSizeValue);
            break;
        }

        cssValues.push_back(pair);
    }
}

String* HTMLFontElement::color()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_color);
}

void HTMLFontElement::setColor(String* color)
{
    setAttribute(starFish()->staticStrings()->m_color, color);
}

String* HTMLFontElement::size()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_size);
}

void HTMLFontElement::setSize(String* size)
{
    setAttribute(starFish()->staticStrings()->m_size, size);
}
}
