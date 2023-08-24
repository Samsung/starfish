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

#if defined(STARFISH_SERVICE_WORKER_HOST) && \
    !defined(__StarfishServiceWorkerScriptController__)
#define __StarfishServiceWorkerScriptController__

#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace Starfish {

class RegistrationStore;
class ServiceWorkerScriptController;

class ServiceWorkerScriptControllerClient : public ResourceRequestClient {
public:
    ServiceWorkerScriptControllerClient(
        ServiceWorkerScriptController* serviceWorkerScriptController)
        : m_serviceWorkerScriptController(serviceWorkerScriptController)
        , m_scriptLoadResult(ScriptLoadResult::NotHandled)
    {
    }

    void onProgressEvent(ResourceRequest* request, bool isExplicitAction);

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit);

    ScriptLoadResult scriptLoadResult()
    {
        return m_scriptLoadResult;
    }

private:
    ServiceWorkerScriptController* m_serviceWorkerScriptController;
    ScriptLoadResult m_scriptLoadResult;
};

class ServiceWorkerScriptController : public WorkerScriptController {
public:
    ServiceWorkerScriptController(ExecutionContext* executionContext,
                                  RegistrationStore* registrationStore);

    ScriptLoadResult loadJavaScript(ResourceURL* resourceURL) override;

    ScriptLoadResult loadJavaScriptFromCache(ResourceURL* resourceURL);

    RegistrationStore* registrationStore()
    {
        return m_registrationStore;
    }

private:
    RegistrationStore* m_registrationStore;
};
} // namespace Starfish

#endif
