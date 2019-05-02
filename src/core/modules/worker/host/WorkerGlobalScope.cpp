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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "Starfish.h"

#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerLocation.h"
#include "core/modules/worker/host/WorkerNavigator.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/ErrorEvent.h"
#include "core/dom/DOMException.h"

namespace Starfish {

WorkerGlobalScope::WorkerGlobalScope(WebWorker* webWorker, ResourceURL* url,
                                     String* charSet)
    : EventTarget()
    , GlobalScope(webWorker)
    , m_webWorker(webWorker)
{
    STARFISH_ASSERT(webWorker != nullptr && url != nullptr &&
                    charSet != nullptr);

    m_scriptBindingInstance = new ScriptBindingWorkerInstance(
        webWorker->scriptEngineInstance(), this);

    m_executionContext = new ExecutionContext(this, m_scriptBindingInstance,
                                              url, charSet, this, false);
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
    if (m_scriptBindingInstance) {
        m_scriptBindingInstance->destroy();
    }
}

void WorkerGlobalScope::importScripts(GCVector<String*>& urls)
{
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
}

#endif /* STARFISH_WEBWORKER_HOST */
