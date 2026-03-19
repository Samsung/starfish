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
#include "core/dom/HTMLTableColElement.h"
#include "core/dom/HTMLTableCellElement.h"

namespace Starfish {
void HTMLTableColElement::didAttributeChanged(QualifiedName name,
                                              Optional<String*> old,
                                              String* value,
                                              bool attributeCreated,
                                              bool attributeRemoved)
{
    HTMLTablePartElement::didAttributeChanged(
        name, old, value, attributeCreated, attributeRemoved);
    if (name == starfish()->staticStrings()->m_width) {
        setNeedsStyleRecalc();
    }
}

void HTMLTableColElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    HTMLTablePartElement::styleForPresentationAttribute(cssValues, matchedRules,
                                                        cssCustomValues);

    String* w = getAttributeOrEmpty(starfish()->staticStrings()->m_width);
    if (!w->equals(String::emptyString)) {
        // Use px as the default unit
        // There is another type (relative_length), but it is not implemented in
        // any of major browsers.
        if (!w->contains("px") && !w->contains("%")) {
            w = w->concat(String::createASCIIString("px"));
        }

        CSSStyleValuePair pair;
        CSSTokenVector tokens;
        CSSTokenValue token(w->toUTF8NonGCString());
        tokens.push_back(token);
        if (pair.updateValueWidth(document(), tokens)) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Width);
            cssValues.push_back(pair);
        }
    }
}

void HTMLTableColElement::setSpan(uint32_t span)
{
    setAttribute(starfish()->staticStrings()->m_span, String::fromInt(span));
}

uint32_t HTMLTableColElement::span()
{
    Optional<String*> span = getAttribute(starfish()->staticStrings()->m_span);
    if (span.hasValue()) {
        int spanVal = String::parseInt(span.getValue());
        if (spanVal < 1) {
            return 1;
        }
        if (spanVal > HTMLTableCellElement::MAX_COLSPAN) {
            return HTMLTableCellElement::MAX_COLSPAN;
        }
        return spanVal;
    }

    return 1;
}

String* HTMLTableColElement::ch()
{
    Optional<String*> ret = getAttribute(starfish()->staticStrings()->m_char);
    if (ret.hasValue()) {
        return ret.getValue();
    }
    // TODO : Set defualt Value that is the decimal point character for the
    // current language as set by the lang attribute
    STARFISH_UNSUPPORTED("HTMLTableColElement property: ch");
    return String::createASCIIString(".");
}

void HTMLTableColElement::setCh(String* ch)
{
    setAttribute(starfish()->staticStrings()->m_char, ch);
}
} // namespace Starfish
