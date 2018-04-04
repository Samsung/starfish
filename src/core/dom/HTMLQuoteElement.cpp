/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/HTMLQuoteElement.h"

namespace StarFish {
HTMLQuoteElement::HTMLQuoteElement(Document* document, AtomicString name)
    : HTMLElement(document)
    , m_name(starFish()->staticStrings()->m_xhtmlNamespaceURI, name)
{
}

void* HTMLQuoteElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLQuoteElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLQuoteElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLQuoteElement::setCite(String* cite)
{
    setAttribute(starFish()->staticStrings()->m_cite, cite);
}

String* HTMLQuoteElement::cite()
{
    if (hasAttribute(starFish()->staticStrings()->m_cite) != SIZE_MAX) {
        return (new ResourceURL(
                    getAttributeOrEmpty(starFish()->staticStrings()->m_cite),
                    document()->baseURI()))
            ->urlString();
    } else {
        return String::emptyString;
    }
}
}
