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
#include "core/dom/HTMLHeadingElement.h"

namespace StarFish {
HTMLHeadingElement::HTMLHeadingElement(Document* document, AtomicString name)
    : HTMLElement(document)
    , m_name(starFish()->staticStrings()->m_xhtmlNamespaceURI, name)
{
}

void* HTMLHeadingElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLHeadingElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLHeadingElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLHeadingElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLHeadingElement::align()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_align);
}

void HTMLHeadingElement::setAlign(String* align)
{
    setAttribute(starFish()->staticStrings()->m_align, align);
}

void HTMLHeadingElement::didAttributeChanged(QualifiedName name, String* old,
                                             String* value,
                                             bool attributeCreated,
                                             bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_align) {
        setNeedsStyleRecalc();
    }
}

void HTMLHeadingElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* value = align()->toASCIILower();
    if (!value->isEmpty()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::TextAlignValueKind);

        if (value->equals("start")) {
            pair.setValue(TextAlignValue::StartTextAlignValue);
        } else if (value->equals("end")) {
            pair.setValue(TextAlignValue::EndTextAlignValue);
        } else if (value->equals("left")) {
            pair.setValue(TextAlignValue::LeftTextAlignValue);
        } else if (value->equals("center")) {
            pair.setValue(TextAlignValue::CenterTextAlignValue);
        } else if (value->equals("right")) {
            pair.setValue(TextAlignValue::RightTextAlignValue);
        }

        cssValues.push_back(pair);
    }
}
}
