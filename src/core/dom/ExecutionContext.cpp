/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "core/modules/profiling/Profiling.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/util/String.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/EventTarget.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/WebOrigin.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/networking/WebSocket.h"
#include "core/modules/worker/util/Trace.h"

namespace Starfish {

ExecutionContext::ExecutionContext(GlobalScope* globalScope,
                                   ScriptBindingInstance* instance,
                                   ResourceURL* uri, String* charSet,
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
    , m_characterSet(charSet)
    , m_webOrigin(WebOrigin::createDocumentOrigin(uri))
    , m_contentSecurityPolicy(new ContentSecurityPolicy(this))
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

WorkerGlobalScope* ExecutionContext::workerGlobalScope()
{
    STARFISH_ASSERT(!hasDocument());
    return static_cast<WorkerGlobalScope*>(m_documentOrWorkerGlobalScope);
}

Starfish* ExecutionContext::starfish() const
{
    return m_globalScope->webBase()->starfish();
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

void ExecutionContext::addPointerInRootSet(void* ptr)
{
    STARFISH_ASSERT(globalScope()->isContextThread());

    auto iter = m_rootMap.find(ptr);
    if (iter == m_rootMap.end()) {
        m_rootMap.insert(std::make_pair(ptr, 1));
    } else {
        iter.value()++;
    }
}

void ExecutionContext::removePointerFromRootSet(void* ptr)
{
    STARFISH_ASSERT(globalScope()->isContextThread());

    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        if (iter->second == 1) {
            m_rootMap.erase(iter);
        } else {
            iter.value()--;
        }
    }
}

void ExecutionContext::clearPointerRootMap()
{
    m_rootMap.clear();
}

#ifndef NDEBUG
size_t ExecutionContext::countPointersInRootSet(void* ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter != m_rootMap.end()) {
        return iter->second;
    } else {
        return 0;
    }
}
#endif

void ExecutionContext::addActiveResourceRequests(ResourceRequest* request)
{
    m_activeResourceRequests.push_back(request);
}

void ExecutionContext::removeActiveResourceRequests(ResourceRequest* request)
{
    auto iter = std::find(m_activeResourceRequests.begin(),
                          m_activeResourceRequests.end(), request);
    if (iter != m_activeResourceRequests.end()) {
        m_activeResourceRequests.erase(iter);
    }
}

void ExecutionContext::disposeActiveResourceRequests()
{
    while (m_activeResourceRequests.size()) {
        m_activeResourceRequests.back()->abort();
    }
}
#ifdef STARFISH_ENABLE_WEBSOCKET
void ExecutionContext::addActiveWebSockets(WebSocket* webSocket)
{
    m_activeWebSockets.push_back(webSocket);
}

void ExecutionContext::removeActiveWebSockets(WebSocket* webSocket)
{
    auto iter = std::find(m_activeWebSockets.begin(), m_activeWebSockets.end(),
                          webSocket);
    if (iter != m_activeWebSockets.end()) {
        m_activeWebSockets.erase(iter);
    }
}

void ExecutionContext::disposeActiveWebSockets()
{
    while (m_activeWebSockets.size()) {
        m_activeWebSockets.back()->dispose();
    }
}
#endif
void ExecutionContext::initContentSecurityPolicy(
    ContentSecurityPolicy* inheritedPolicy)
{
    m_contentSecurityPolicy->copyFrom(inheritedPolicy);
}

void ExecutionContext::dispatchEventIdleTimeByUA(Event* event)
{
    webBase()->messageLoop()->addIdler(
        globalScope(),
        [](size_t handle, void* data0, void* data1) {
            ((EventTarget*)data0)->dispatchEventByUA((Event*)data1);
        },
        m_documentOrWorkerGlobalScope, event);
}

#ifdef STARFISH_ENABLE_SERVICE_WORKER
ServiceWorker* ExecutionContext::activeServiceWorker() const
{
    TRACE_SCOPE(CLIENT);
    return m_activeServiceWorker;
};

void ExecutionContext::setActiveServiceWorker(ServiceWorker* serviceWorker)
{
    TRACE_SCOPE(CLIENT);
    m_activeServiceWorker = serviceWorker;
}
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
} // namespace Starfish
