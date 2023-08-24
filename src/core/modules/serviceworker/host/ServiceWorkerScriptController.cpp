/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_SERVICE_WORKER_HOST

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/serviceworker/host/ServiceWorkerScriptController.h"
#include "core/modules/serviceworker/RegistrationStore.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

namespace Starfish {

void ServiceWorkerScriptControllerClient::onProgressEvent(
    ResourceRequest* request, bool isExplicitAction)
{
    STARFISH_ASSERT(request != nullptr);
    ProgressState progState = request->progressState();
}

void ServiceWorkerScriptControllerClient::onReadyStateChange(
    ResourceRequest* request, bool fromExplicit)
{
    STARFISH_ASSERT(request != nullptr);

    if (!fromExplicit) {
        return;
    }

    if (request->readyState() == ReadyState::Done) {
        if (request->isError() == false && request->status() == 200) {
            auto& response = request->response();
            String* text = String::fromUTF8(response.data(), response.size());
            if (m_serviceWorkerScriptController->evaluatefromString(text)) {
                m_scriptLoadResult = ScriptLoadResult::Success;

                m_serviceWorkerScriptController->registrationStore()
                    ->saveWorkerScripts(
                        m_serviceWorkerScriptController->executionContext()
                            ->baseURL()
                            ->baseURI(),
                        m_serviceWorkerScriptController->executionContext()
                            ->urlString(),
                        text);

            } else {
                m_scriptLoadResult = ScriptLoadResult::ScriptError;
            }

            response.clear();
            response.shrink_to_fit();

        } else {
            m_scriptLoadResult = ScriptLoadResult::NetworkError;
        }
    }
}

ServiceWorkerScriptController::ServiceWorkerScriptController(
    ExecutionContext* executionContext, RegistrationStore* registrationStore)
    : WorkerScriptController(executionContext)
    , m_registrationStore(registrationStore)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

ScriptLoadResult ServiceWorkerScriptController::loadJavaScript(
    ResourceURL* resourceURL)
{
    return loadJavaScriptInternal<ServiceWorkerScriptControllerClient,
                                  ServiceWorkerScriptController>(resourceURL,
                                                                 this);
}

ScriptLoadResult ServiceWorkerScriptController::loadJavaScriptFromCache(
    ResourceURL* resourceURL)
{
    String* scope = resourceURL->baseURI();
    auto script = m_registrationStore->loadWorkerScript(scope);
    if (!script.hasValue()) {
        return ScriptLoadResult::FileError;
    }

    if (!evaluatefromString(script.getValue())) {
        return ScriptLoadResult::ScriptError;
    }

    return ScriptLoadResult::Success;
}

} // namespace Starfish

#endif /* STARFISH_SERVICE_WORKER_HOST */
