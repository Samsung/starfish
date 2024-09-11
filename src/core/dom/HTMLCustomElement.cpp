/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "HTMLCustomElement.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/CustomElementRegistry.h"

namespace Starfish {

void* HTMLCustomElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLCustomElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLCustomElement)] = { 0 };
        HTMLCustomElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLCustomElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLCustomElement::init(ScriptBindingInstance* instance,
                             void* domObjectPointer)
{
    HTMLElement::init(instance, domObjectPointer);

    ScriptValue proto = getScriptObjectPropertyThrowsException(
        instance,
        scriptValueAsObject(
            m_customElementRegistryData->constructor->scriptValue()),
        scriptStringPrototype(instance));
    setScriptObjectProperty(instance, scriptObject(),
                            scriptString__proto__(instance), proto);
}

} // namespace Starfish
