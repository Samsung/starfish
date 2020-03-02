/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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
#include "core/dom/HTMLDialogElement.h"

#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {
void* HTMLDialogElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLDialogElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLDialogElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLDialogElement, m_returnValue));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLDialogElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLDialogElement::show()
{
    if (hasAttribute(starfish()->staticStrings()->m_open) != SIZE_MAX) {
        return;
    }

    setAttribute(starfish()->staticStrings()->m_open, String::emptyString);
}

void HTMLDialogElement::showModal()
{
    if (hasAttribute(starfish()->staticStrings()->m_open) != SIZE_MAX) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    setAttribute(starfish()->staticStrings()->m_open, String::emptyString);
}

void HTMLDialogElement::close(String* returnValue)
{
    if (hasAttribute(starfish()->staticStrings()->m_open) == SIZE_MAX) {
        return;
    }

    removeAttribute(starfish()->staticStrings()->m_open);

    if (returnValue->equals(String::emptyString)) {
        m_returnValue = returnValue;
    }

    String* eventType =
        executionContext()->starfish()->staticStrings()->m_close.localName();
    Event* e = new Event(executionContext(), eventType);
    dispatchEventByUA(e);
}
} // namespace Starfish
