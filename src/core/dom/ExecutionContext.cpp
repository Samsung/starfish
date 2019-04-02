/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishBase.h"
#include "core/modules/profiling/Profiling.h"
#include "core/util/String.h"
#include "core/page/GlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {

ExecutionContext::ExecutionContext(GlobalScope* globalScope,
                                   ScriptBindingInstance* instance,
                                   ResourceURL* uri,
                                   void* documentOrWorkerGlobalScope,
                                   bool hasDocument)
    : m_globalScope(globalScope)
    , m_documentOrWorkerGlobalScope(documentOrWorkerGlobalScope)
    , m_hasDocument(hasDocument)
    , m_scriptBindingInstance(instance)
    , m_createdTick(longTickCount())
    , m_documentURI(uri)
    , m_referrer(nullptr)
    , m_baseURL(nullptr)
{
}

void* ExecutionContext::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ExecutionContext));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ExecutionContext)] = { 0 };
        ExecutionContext::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ExecutionContext));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Document* ExecutionContext::document()
{
    STARFISH_ASSERT(hasDocument());
    return static_cast<Document*>(m_documentOrWorkerGlobalScope);
}

WebBase* ExecutionContext::webBase() const
{
    return m_globalScope->webBase();
}

String* ExecutionContext::urlString()
{
    return m_documentURI->urlString();
}

String* ExecutionContext::referrer()
{
    if (!m_referrer) {
        return String::emptyString;
    }
    return m_referrer->urlString();
}

ResourceURL* ExecutionContext::baseURL() const
{
    // If there is no base element that has an href attribute in the Document,
    // then return the Document's fallback base URL.
    if (m_baseURL) {
        return m_baseURL;
    }
    return ResourceURL::aboutBlankURL();
}

#ifdef STARFISH_ENABLE_SERVICE_WORKER
ServiceWorker* ExecutionContext::activeServiceWorker() const
{
    return m_activeServiceWorker;
};

void ExecutionContext::setActiveServiceWorker(ServiceWorker* serviceWorker)
{
    m_activeServiceWorker = serviceWorker;
}
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
}
