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

#include "core/dom/HTMLButtonElement.h"

#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/page/BrowsingContext.h"

namespace Starfish {

HTMLButtonElement::HTMLButtonElement(Document* document,
                                     const QualifiedName& qname)
    : HTMLFormControl(document, qname)
{
}

void* HTMLButtonElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLButtonElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLButtonElement)] = { 0 };
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLButtonElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLButtonElement::type()
{
    String* typeAttr = getAttributeOrEmpty(starfish()->staticStrings()->m_type);
    typeAttr = typeAttr->toASCIILower();

    if (typeAttr->equals("submit")) {
        return typeAttr;
    } else if (typeAttr->equals("reset")) {
        return typeAttr;
    } else if (typeAttr->equals("button")) {
        return typeAttr;
    }

    return starfish()->staticStrings()->m_submit.localName();
}

bool HTMLButtonElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (event->isMouseEvent() || event->isTouchEvent()) {
        if (event->type()->equalsIgnoreCase("click")) {
            if (formOwner()) {
                if (type()->equals("submit")) {
                    fireSubmitEvent();
                } else if (type()->equals("reset")) {
                    // TODO
                }
                return true;
            }
        }
    }

    return false;
}
} // namespace Starfish
