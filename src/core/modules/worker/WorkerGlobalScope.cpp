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

#ifdef STARFISH_ENABLE_WORKER

#include "StarfishConfig.h"
#include "Starfish.h"

#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/worker/WebWorker.h"
#include "core/modules/worker/WorkerGlobalScope.h"
#include "core/modules/worker/WorkerLocation.h"
#include "core/modules/worker/WorkerNavigator.h"
#include "core/modules/worker/WorkerScriptController.h"
#include "core/util/debug/Trace.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/ErrorEvent.h"
#include "core/dom/DOMException.h"

#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/IDBFactory.h"

#include "core/page/WindowOrWorkerGlobalScope.h"
#include "core/dom/StructuredSerializeOptions.h"

namespace Starfish {

WorkerGlobalScope::WorkerGlobalScope(WebWorker* webWorker)
    : EventTarget()
    , GlobalScope(webWorker)
    , m_webWorker(webWorker)
    , m_closing(false)
{
    STARFISH_ASSERT(webWorker != nullptr);
}

void WorkerGlobalScope::initGlobalScope(ResourceURL* url, String* charSet)
{
    STARFISH_ASSERT(url != nullptr);
    STARFISH_ASSERT(charSet != nullptr);

    m_executionContext = new ExecutionContext(this, m_scriptBindingInstance,
                                              url, charSet, this, false);
    /*
        baseURL: empty
        origin: http://localhost:1111
        url: http://localhost:1111/sw.js
    */
    TRACE(HOST, "baseURL:", CSTR(url->baseURL()));
    TRACE(HOST, "origin:", CSTR(url->origin()));
    TRACE(HOST, "url:", CSTR(url->urlString()));

    m_executionContext->setBaseURL(new ResourceURL(url->baseURL()));
    m_workerScriptController = new WorkerScriptController(m_executionContext);
    m_workerLocation = new WorkerLocation(m_executionContext, url);
    m_workerNavigator = new WorkerNavigator(m_executionContext);
    // Same device API exposure rule as Window.
    m_scriptBindingInstance->initBinding(url->isFileURL());
}

DedicatedWorkerGlobalScope* WorkerGlobalScope::asDedicatedWorkerGlobalScope()
{
    STARFISH_ASSERT(isDedicatedWorkerGlobalScope());

    return reinterpret_cast<DedicatedWorkerGlobalScope*>(this);
}

void WorkerGlobalScope::dispatchErrorEvent(ErrorEventInit& errorInfo)
{
    Event* errorEvent = new ErrorEvent(
        m_executionContext, staticStrings()->m_error.localName(), errorInfo);
    dispatchEventByUA(errorEvent);
}

bool WorkerGlobalScope::terminate()
{
    STARFISH_ASSERT(m_executionContext->isContextThread());

    if (m_closing) {
        return false;
    }

    m_closing = true;

    m_webWorker->messageLoop()->clearPendingIdlers(nullptr);

    return true;
}

void WorkerGlobalScope::dispose()
{
#if !defined(STARFISH_WEBWORKER_HOST)
    m_executionContext->disposeActiveWorkers();
#endif
#ifdef STARFISH_ENABLE_WEBSOCKET
    // A socket's service thread posts to this scope's loop, which goes away
    // right after the scope; close and join the sockets first, then drop the
    // tasks they posted while closing so nothing runs on a torn-down scope.
    // The root map goes with the scope, as it does for a document.
    m_executionContext->disposeActiveWebSockets();
    m_webWorker->messageLoop()->clearPendingIdlers(nullptr);
    m_executionContext->clearPointerRootMap();
#endif
    if (m_scriptBindingInstance != nullptr) {
        m_scriptBindingInstance->destroy();
        m_scriptBindingInstance = nullptr;
    }
}

uint32_t WorkerGlobalScope::setTimeout(TimerHandler handler, int32_t delay,
                                       void* data)
{
    STARFISH_ASSERT(handler != nullptr);
    STARFISH_ASSERT(data != nullptr);
    return webWorker()->timer()->addTimer(std::max(0, delay), this, handler,
                                          data, false);
}

void WorkerGlobalScope::clearTimeout(int32_t id)
{
    webWorker()->timer()->removeTimer(id);
}

uint32_t WorkerGlobalScope::setInterval(TimerHandler handler, int32_t delay,
                                        void* data)
{
    STARFISH_ASSERT(handler != nullptr);
    STARFISH_ASSERT(data != nullptr);
    return webWorker()->timer()->addTimer(std::max(0, delay), this, handler,
                                          data, true);
}

void WorkerGlobalScope::clearInterval(int32_t id)
{
    webWorker()->timer()->removeTimer(id);
}

void WorkerGlobalScope::importScripts(GCVector<String*>& urls)
{
    TRACE_SCOPE(HOST);
    if (urls.empty()) {
        return;
    }

    for (unsigned i = 0; i < urls.size(); i++) {
        STARFISH_ASSERT(urls[i] != nullptr);
        if (urls[i]->isEmpty() == false) {
            ResourceURL* url = new ResourceURL(
                urls[i], executionContext()->baseURL()->baseURI());
            importScript(url);
        }
    }
}

void WorkerGlobalScope::importScript(ResourceURL* url)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(url != nullptr);
    if (url->isValid() == false) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SYNTAX_ERR, "Invalid URL");
    }

    ScriptLoadResult result = m_workerScriptController->loadJavaScript(url);
    if (result != ScriptLoadResult::Success) {
        DOMException::Code errorCode = DOMException::Code::NOT_SUPPORTED_ERR;
        if (result == ScriptLoadResult::NetworkError) {
            errorCode = DOMException::Code::NETWORK_ERR;
        } else if (result == ScriptLoadResult::ScriptError) {
            errorCode = DOMException::SCRIPT_ERROR;
        }
        throw new DOMException(executionContext(), errorCode,
                               "Failed to execute 'importScript'");
    }
}

void WorkerGlobalScope::importModuleScript(ResourceURL* url)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(url != nullptr);
    if (url->isValid() == false) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SYNTAX_ERR, "Invalid URL");
    }

    ScriptLoadResult result =
        m_workerScriptController->loadJavaScriptModule(url);
    if (result != ScriptLoadResult::Success) {
        throw new DOMException(executionContext(),
                               DOMException::Code::DOM_EXCEPTION,
                               "Failed to load module script");
    }
}

bool WorkerGlobalScope::loadMainScript(WorkerType type)
{
    TRACE(WORKER, "load main script");

    if (m_inlineMainScript) {
        String* source = m_inlineMainScript;
        m_inlineMainScript = nullptr;
        return m_workerScriptController->evaluatefromString(source);
    }

    try {
        if (type == WorkerType::Module) {
            importModuleScript(executionContext()->documentURI());
        } else {
            importScript(executionContext()->documentURI());
        }
    } catch (DOMException* e) {
        return false;
    }

    return true;
}

Promise* WorkerGlobalScope::fetch(RequestInfo& input)
{
    TRACE_SCOPE(HOST);
    return Fetch::fetch(executionContext(), input);
}

Promise* WorkerGlobalScope::fetch(RequestInfo& input, RequestInit& init)
{
    TRACE_SCOPE(HOST);
    return Fetch::fetch(executionContext(), input, init);
}

String* WorkerGlobalScope::btoa(ExecutionContext* executionContext,
                                String* data)
{
    TRACE_SCOPE(HOST);
    return WindowOrWorkerGlobalScope::btoa(executionContext, data);
}

String* WorkerGlobalScope::atob(ExecutionContext* executionContext,
                                String* data)
{
    TRACE_SCOPE(HOST);
    return WindowOrWorkerGlobalScope::atob(executionContext, data);
}

void WorkerGlobalScope::queueMicrotask(ExecutionContext* executionContext,
                                       ScriptObject callback)
{
    WindowOrWorkerGlobalScope::queueMicrotask(executionContext, callback);
}

ScriptValue WorkerGlobalScope::structuredClone(
    ExecutionContext* executionContext, ScriptValue value)
{
    return WindowOrWorkerGlobalScope::structuredClone(executionContext, value);
}

ScriptValue WorkerGlobalScope::structuredClone(
    ExecutionContext* executionContext, ScriptValue value,
    StructuredSerializeOptions options)
{
    return WindowOrWorkerGlobalScope::structuredClone(executionContext, value,
                                                      options);
}

#ifdef STARFISH_ENABLE_CANVAS
Promise* WorkerGlobalScope::createImageBitmap(
    ExecutionContext* executionContext, ImageBitmapSource image,
    ImageBitmapOptions options)
{
    return WindowOrWorkerGlobalScope::createImageBitmap(executionContext, image,
                                                        options);
}

Promise* WorkerGlobalScope::createImageBitmap(
    ExecutionContext* executionContext, ImageBitmapSource image, int32_t sx,
    int32_t sy, int32_t sw, int32_t sh, ImageBitmapOptions options)
{
    return WindowOrWorkerGlobalScope::createImageBitmap(
        executionContext, image, sx, sy, sw, sh, options);
}
#endif

Performance* WorkerGlobalScope::performance()
{
    TRACE_SCOPE(HOST);
    return Performance::create(executionContext());
}

#if defined(STARFISH_ENABLE_IDB)
IDBFactory* WorkerGlobalScope::indexedDB()
{
    if (!m_idbFactory) {
        IDBStorageManager::instance().start();
        m_idbFactory = new IDBFactory(executionContext());
    }

    return m_idbFactory;
}
#endif

DEFINE_EVENT_LISTENER(WorkerGlobalScope, error);

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
