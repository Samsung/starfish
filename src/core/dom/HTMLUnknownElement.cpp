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
#include "core/dom/Document.h"
#include "core/dom/HTMLUnknownElement.h"
#include "core/dom/HTMLCustomElement.h"
#include "core/dom/CustomElementRegistry.h"

namespace Starfish {
HTMLUnknownElement::HTMLUnknownElement(Document* document,
                                       const QualifiedName& qname)
    : HTMLElement(document, qname)
    , m_customElementRegistryDataPlaceHolder(nullptr)
{
}

void HTMLUnknownElement::morphIntoCustomElement(CustomElementRegistryData* data)
{
    m_customElementRegistryDataPlaceHolder = data;

    // change c++ vptr to HTMLCustomElement
    HTMLCustomElement vptrSource(document(), data->name, data);
    size_t* srcPtr = reinterpret_cast<size_t*>(&vptrSource);
    size_t* thisPtr = reinterpret_cast<size_t*>(this);
    *thisPtr = *srcPtr;
}

void* HTMLUnknownElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLUnknownElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLUnknownElement)] = { 0 };
        HTMLUnknownElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLUnknownElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

static_assert(sizeof(HTMLCustomElement) == sizeof(HTMLUnknownElement), "");
} // namespace Starfish
