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
#include "core/modules/message_loop/Timer.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/worker/host/WorkerLocation.h"
#include "core/modules/worker/host/WorkerNavigator.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/ErrorEvent.h"
#include "core/dom/DOMException.h"

#include "core/page/WindowOrWorkerGlobalScope.h"

namespace Starfish {

WorkerGlobalScope::WorkerGlobalScope(WebWorker* webWorker)
    : EventTarget()
    , GlobalScope(webWorker)
    , m_webWorker(webWorker)
    , m_crypto(nullptr)
{
    STARFISH_ASSERT(webWorker != nullptr);
}

WorkerGlobalScope::WorkerGlobalScope(WebWorker* webWorker, ResourceURL* url,
                                     String* charSet)
    : EventTarget()
    , GlobalScope(webWorker)
    , m_webWorker(webWorker)
    , m_crypto(nullptr)
{
    TRACE_SCOPE(HOST);

    STARFISH_ASSERT(webWorker != nullptr);
    STARFISH_ASSERT(url != nullptr);
    STARFISH_ASSERT(charSet != nullptr);

    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<WorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);

    initGlobalScope(url, charSet);
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
    m_scriptBindingInstance->initBinding();
}

void WorkerGlobalScope::dispatchErrorEvent(ErrorEventInit& errorInfo)
{
    Event* errorEvent = new ErrorEvent(
        m_executionContext, staticStrings()->m_error.localName(), errorInfo);
    dispatchEventByUA(errorEvent);
}

void WorkerGlobalScope::dispose()
{
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
    return webWorker()->timer()->addTimer(delay, this, handler, data, false);
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
    return webWorker()->timer()->addTimer(delay, this, handler, data, true);
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
        throw new DOMException(executionContext(),
                               DOMException::Code::DOM_EXCEPTION,
                               "Failed to execute 'importScript'");
    }
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

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
