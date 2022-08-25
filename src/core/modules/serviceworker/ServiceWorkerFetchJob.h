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

#if defined(STARFISH_WEBWORKER_HOST) && !defined(__ServiceWorkerFetchJob__)
#define __ServiceWorkerFetchJob__

#include "StarfishConfig.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class RequestData;
class Response;
class FetchEvent;
class Promise;
class ServiceWorkerRegistration;
class ServiceWorkerGlobalScope;
class ServiceWorkerHostConnection;

class ServiceWorkerFetchJob : public gc {
public:
    ServiceWorkerFetchJob(ServiceWorkerGlobalScope* client,
                          ServiceWorkerContextId contextId,
                          ServiceWorkerHostConnection* connection)
        : m_client(client)
        , m_connection(connection)
        , m_contextId(contextId)
    {
    }

    Nullable<Response*> handleFetch(RequestData* requestData);

    void onCompleteFetch(FetchEvent* event);

    void failJob();
    void successJob();

    DEFINE_GETTER(ServiceWorkerGlobalScope*, client);
    DEFINE_GETTER(ServiceWorkerHostConnection*, connection);
    DEFINE_GETTER(ServiceWorkerContextId, contextId);
    DEFINE_GETTER(String*, url);
    DEFINE_GETTER_SETTER(bool, handleFetchFailed, HandleFetchFailed);
    DEFINE_GETTER_SETTER(bool, respondWithEntered, RespondWithEntered);
    DEFINE_GETTER_SETTER(bool, eventCanceled, EventCanceled);
    DEFINE_GETTER_SETTER(Response*, response, Response);
    DEFINE_GETTER_SETTER(Promise*, eventHandled, EventHandled);

private:
    ServiceWorkerGlobalScope* m_client;
    ServiceWorkerHostConnection* m_connection;
    ServiceWorkerContextId m_contextId;
    String* m_url{ nullptr };
    bool m_handleFetchFailed{ false };
    bool m_respondWithEntered{ false };
    bool m_eventCanceled{ false };
    Response* m_response{ nullptr };
    Promise* m_eventHandled{ nullptr };
    ServiceWorkerRegistration* m_registration{ nullptr };
};

} // namespace Starfish

#endif
