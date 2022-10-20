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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishFetchEventData__)
#define __StarfishFetchEventData__

#include "core/util/Archivable.h"
#include "core/util/Archiver.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace Starfish {

class Response;

class FetchEventRequestData : public Archivable {
public:
    // data
    ServiceWorkerContextId contextId;
    ServiceWorkerFetchKey fetchTaskId;
    String* baseURL;
    String* url;
    String* scopeURL;
    RequestDestination destination;
    String* method;
    std::unordered_map<std::string, std::string> headerMap;

    DEFINE_ARCHIVE_ID_GETTER(FetchEventRequestData);

    void archive(Archiver& ar) override
    {
        ar.MemberId("contextId", contextId);
        ar.Member("fetchTaskId") & fetchTaskId;
        ar.Member("baseURL") & baseURL;
        ar.Member("url") & url;
        ar.Member("scopeURL") & scopeURL;
        ar.MemberEnum("destination", destination);
        ar.Member("method") & method;
        ar.Member("headerMap") & headerMap;
    }

    static FetchEventRequestData* createFetchEventRequestData(
        ResourceRequest* request);
    RequestData* toRequestData();
};

class FetchEventResponseData : public Archivable {
public:
    // data
    ServiceWorkerContextId contextId;
    ServiceWorkerFetchKey fetchTaskId;
    String* url;
    bool isCached{ false };
    bool isSuccessful{ false };
    std::string cachePath;
    std::string responsePath;

    DEFINE_ARCHIVE_ID_GETTER(FetchEventResponseData);

    void archive(Archiver& ar) override
    {
        ar.MemberId("contextId", contextId);
        ar.Member("fetchTaskId") & fetchTaskId;
        ar.Member("url") & url;
        ar.Member("isCached") & isCached;
        ar.Member("isSuccessful") & isSuccessful;
        ar.Member("cachePath") & cachePath;
        ar.Member("responsePath") & responsePath;
    }

    static FetchEventResponseData* createFetchEventResponseData(
        ServiceWorkerContextId id, ServiceWorkerFetchKey fetchTaskId,
        Response* response);
};

} // namespace Starfish

#endif
