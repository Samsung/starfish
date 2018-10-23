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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLModElement.h"

namespace Starfish {
HTMLModElement::HTMLModElement(Document* document, const QualifiedName& qname)
    : HTMLElement(document, qname)
{
}

void* HTMLModElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLModElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLModElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLModElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLModElement::setCite(String* cite)
{
    setAttribute(starfish()->staticStrings()->m_cite, cite);
}

String* HTMLModElement::cite()
{
    if (hasAttribute(starfish()->staticStrings()->m_cite) != SIZE_MAX) {
        return (new ResourceURL(
                    getAttributeOrEmpty(starfish()->staticStrings()->m_cite),
                    document()->baseURI()))
            ->urlString();
    } else {
        return String::emptyString;
    }
}

void HTMLModElement::setDateTime(String* dateTime)
{
    setAttribute(starfish()->staticStrings()->m_datetime, dateTime);
}

String* HTMLModElement::dateTime()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_datetime);
}
}
