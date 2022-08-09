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
    !defined(__StarfishServiceWorkerGlobalScopeProxy__)
#define __StarfishServiceWorkerGlobalScopeProxy__

#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class ServiceWorkerGlobalScope;
class ExtendableEvent;
class FetchEventData;
class RequestData;

class ServiceWorkerGlobalScopeProxy : public gc {
public:
    ServiceWorkerGlobalScopeProxy(ServiceWorkerContextId& id)
        : m_clientContextId(id)
    {
    }

    void setGlobalScope(ServiceWorkerGlobalScope* scope);

    void runPendingEvent();

    void handleFetch(RequestData* data);

private:
    ServiceWorkerContextId m_clientContextId;
    ServiceWorkerGlobalScope* m_globalScope{ nullptr };
    GCVector<RequestData*> m_pendingFetchEventData;
};
} // namespace Starfish

#endif
