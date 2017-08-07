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
#include "core/dom/HTMLHeadingElement.h"

namespace StarFish {
HTMLHeadingElement::HTMLHeadingElement(Document* document, AtomicString name)
    : HTMLElement(document)
    , m_name(starFish()->staticStrings()->m_xhtmlNamespaceURI, name)
{
}

void* HTMLHeadingElement::operator new(size_t size)
{
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
        if (!old->equals(value)) {
            setAttribute(starFish()->staticStrings()->m_align, value);
            setNeedsStyleRecalc();
        }
    }
}

void HTMLHeadingElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* value = align()->toLower();
    if (!value->isEmpty()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::TextAlign);
        pair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);

        if (value->equals("left")) {
            pair.setValue(SideValue::LeftSideValue);
        } else if (value->equals("center")) {
            pair.setValue(SideValue::CenterSideValue);
        } else if (value->equals("right")) {
            pair.setValue(SideValue::RightSideValue);
        }

        cssValues.push_back(pair);
    }
}
}
