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

#include "StarfishConfig.h"

#include "core/dom/ExecutionContext.h"
#include "core/page/GlobalScope.h"
#include "core/fetch/Response.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/serviceworker/FetchEventData.h"

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

namespace Starfish {

FetchEventRequestData* FetchEventRequestData::createFetchEventRequestData(
    ResourceRequest* request)
{
    FetchEventRequestData* data = new FetchEventRequestData();
    data->contextId = request->executionContext()->globalScope()->uid();
    data->baseURL = request->url()->baseURL();
    data->url = request->url()->urlString();
    data->scopeURL = String::emptyString;
    data->destination = request->requestDestination();
    data->method = request->method();
    // TODO: copy other options
    return data;
}

RequestData* FetchEventRequestData::toRequestData()
{
    RequestData* data = new RequestData();
    data->m_url = new ResourceURL(url, baseURL);
    data->m_destination = destination;
    data->m_method = method;
    return data;
}

FetchEventResponseData* FetchEventResponseData::createFetchEventResponseData(
    ServiceWorkerContextId id, Response* response)
{
    auto data = new FetchEventResponseData();
    data->contextId = id;
    data->url = response->url();
    return data;
}

} // namespace Starfish
#endif
