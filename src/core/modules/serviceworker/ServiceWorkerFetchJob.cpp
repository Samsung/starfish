/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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
#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "Starfish.h"

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/fetch/Response.h"
#include "core/fetch/ResponseData.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/FetchEvent.h"
#include "core/modules/serviceworker/ServiceWorkerFetchJob.h"

namespace Starfish {

Nullable<Response*> ServiceWorkerFetchJob::handleFetch(
    ServiceWorkerGlobalScope* client, RequestData* requestData)
{
    TRACE(HOST);

    Response* response = nullptr;
    ServiceWorkerRegistration* registration = nullptr;

    auto preloadResponse =
        new Promise(client->executionContext()->scriptBindingInstance());

    STARFISH_ASSERT(requestData->m_destination !=
                    RequestDestination::ServiceWorker);

    if (requestData->m_destination == RequestDestination::Embed ||
        requestData->m_destination == RequestDestination::Object) {
        return nullptr;
    }

    // 15. Else if request is a non-subresource request, then:
    if (!requestData->isSubresourceRequest()) {
        // TODO
    } else {
        // 16. Else if request is a subresource request, then:
        // TODO
    }

    struct Param : public gc {
        Param(ServiceWorkerGlobalScope* globalScope_, RequestData* data_,
              Promise* preloadResponse_)
            : globalScope(globalScope_)
            , data(data_)
            , preloadResponse(preloadResponse_)
        {
        }
        ServiceWorkerGlobalScope* globalScope;
        RequestData* data;
        Promise* preloadResponse;
    };

    client->webWorker()->messageLoop()->addMicroTask(
        client,
        [](size_t handle, void* data) {
            auto p = static_cast<Param*>(data);
            auto executionContext = p->globalScope->executionContext();
            auto requestData = p->data;
            String* eventType = executionContext->starfish()
                                    ->staticStrings()
                                    ->m_fetch.localName();
            auto event = new FetchEvent(executionContext, eventType);
            auto requestObject = new Request(executionContext, requestData);

            requestObject->headers()->setGuard(Guard::Immutable);

            event->setCancelable(true);
            event->setRequest(requestObject);
            event->setPreloadResponse(p->preloadResponse);
            auto clientId = p->globalScope->uid().toString();
            event->setClientId(
                String::createASCIIString(clientId.data(), clientId.size()));

            TRACE(HOST, "dispatch FetchEvent:",
                  requestData->m_url->urlString()->toUTF8String().data());

            // 24-12. Dispatch e at activeWorker’s global object.
            p->globalScope->dispatchEventByUA(event);
        },
        new Param(client, requestData, preloadResponse));

    // 25. Wait for task to have executed or for handleFetchFailed to be true.

    return response;
}
} // namespace Starfish
#endif
