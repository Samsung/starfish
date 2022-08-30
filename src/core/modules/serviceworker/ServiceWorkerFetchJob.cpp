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

// https://w3c.github.io/ServiceWorker/#on-fetch-request-algorithm
Nullable<Response*> ServiceWorkerFetchJob::handleFetch(RequestData* requestData)
{
    TRACE(HOST);

    m_handleFetchFailed = false;
    m_respondWithEntered = false;
    m_eventCanceled = false;
    m_response = nullptr;
    m_eventHandled = nullptr;
    ServiceWorkerRegistration* registration = nullptr;
    auto scriptBindingInstance =
        client()->executionContext()->scriptBindingInstance();

    auto preloadResponse = new Promise(scriptBindingInstance);

    STARFISH_ASSERT(requestData->m_destination !=
                    RequestDestination::ServiceWorker);

    if (requestData->m_destination == RequestDestination::Embed ||
        requestData->m_destination == RequestDestination::Object) {
        return nullptr;
    }

    if (!requestData->isSubresourceRequest()) {
        // 15. Else if request is a non-subresource request, then:
    } else {
        // 16. Else if request is a subresource request, then:
    }

    m_eventHandled = new Promise(scriptBindingInstance);

    struct Param : public gc {
        Param(RequestData* data_, Promise* preloadResponse_,
              ServiceWorkerFetchJob* job_)
            : data(data_)
            , preloadResponse(preloadResponse_)
            , job(job_)
        {
        }
        RequestData* data;
        Promise* preloadResponse;
        ServiceWorkerFetchJob* job;
    };

    client()->webWorker()->messageLoop()->addMicroTask(
        client(),
        [](size_t handle, void* data) {
            TRACE(HOST);

            auto p = static_cast<Param*>(data);
            auto client = p->job->client();
            auto executionContext = client->executionContext();
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
            auto clientId = client->uid().toString();
            event->setClientId(
                String::createASCIIString(clientId.data(), clientId.size()));
            event->setHandled(p->job->eventHandled());

            event->setFetchJob(p->job);

            TRACE(HOST, "dispatch FetchEvent:",
                  requestData->m_url->urlString()->toUTF8String().data());

            client->dispatchEventByUA(event);

            // 24-13. Invoke Update Service Worker Extended Events Set with
            // activeWorker and e.

            if (event->respondWithEntered()) {
                p->job->setRespondWithEntered(true);
            }

            // 24-16. If response is null, request’s body is not null, and
            // request’s body's source is null, then:
        },
        new Param(requestData, preloadResponse, this));

    // 25. Wait for task to have executed or for handleFetchFailed to be true.

    return m_response;
}

void ServiceWorkerFetchJob::onCompleteFetch(FetchEvent* event)
{
    TRACE(HOST);

    if (event->respondWithError()) {
        setHandleFetchFailed(true);
    } else {
        setResponse(event->potentialResponse());
    }

    if (!respondWithEntered()) {
        if (eventCanceled()) {
            return failJob();
        }
        return;
    }

    if (handleFetchFailed()) {
        return failJob();
    }

    successJob();
}

void ServiceWorkerFetchJob::failJob()
{
    TRACE(HOST);
    if (eventHandled()) {
        auto exception =
            new DOMException(client()->executionContext(),
                             DOMException::NETWORK_ERR, "NetworkError");
        eventHandled()->reject(exception->scriptValue());
    }
}

void ServiceWorkerFetchJob::successJob()
{
    TRACE(HOST);

    if (eventHandled()) {
        eventHandled()->fulfill(scriptUndefined());
    }
}

} // namespace Starfish
#endif
