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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/WebOrigin.h"
#include "core/page/GlobalScope.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/ServiceWorkerFetchTask.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/client/ServiceWorkerClientConnection.h"
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#include "core/modules/serviceworker/FetchEventHandler.h"
#include "core/fetch/Request.h"
#include "core/page/WebBase.h"

namespace Starfish {

FetchEventData* FetchEventData::createFetchEventData(ResourceRequest* request)
{
    FetchEventData* data = new FetchEventData();
    data->contextId = request->executionContext()->globalScope()->uid();
    data->baseURL = request->url()->baseURL();
    data->url = request->url()->urlString();
    data->scopeURL = String::emptyString;
    data->destination = request->requestDestination();
    // TODO: copy other options
    return data;
}

RequestData* FetchEventData::toRequestData()
{
    RequestData* data = new RequestData();
    data->m_url = new ResourceURL(url, baseURL);
    data->m_destination = destination;
    return data;
}

void ServiceWorkerFetchTask::onProgressEvent(ResourceRequest* request,
                                             bool isExplicitAction)
{
    if (request->progressState() == ProgressState::Load) {
        TRACE(CLIENT, "ServiceWorkerFetchTask::Load:",
              request->url()->href()->toUTF8String().data());
        load(request);
    }
}

void ServiceWorkerFetchTask::load(ResourceRequest* request)
{
    // TODO: check service-workers mode
    auto handler =
        ServiceWorkerProcessManager::instance()->findFetchEventHandler(
            request->executionContext()->globalScope()->uid());
    if (handler.hasValue()) {
        handler->addFetch(FetchEventData::createFetchEventData(request));
    }
}

} // namespace Starfish

#endif
