/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
    !defined(__StarfishServiceWorkerTypes__)
#define __StarfishServiceWorkerTypes__

#include "StarfishBase.h"
#include "core/util/String.h"
#include "core/util/Id.h"

namespace Starfish {

class GlobalScope;
class ServiceWorkerJob;
class ExecutionContext;
class ServiceWorkerRequest;
class ServiceWorkerRegistration;

// https://w3c.github.io/ServiceWorker/#serviceworker
enum class ServiceWorkerState : unsigned {
    Parsed,
    Installing,
    Installed,
    Activating,
    Activated,
    Redundant,
};

enum class ServiceWorkerRegistrationState : unsigned {
    Installing = 0,
    Waiting,
    Active,
};

enum class ServiceWorkerUpdateViaCache : uint8_t {
    Imports,
    All,
    None,
};

enum class ServiceWorkerJobType : unsigned {
    Register,
    Unregister,
    Update,
};

class TypeUtils {
public:
    static Optional<ServiceWorkerUpdateViaCache> stringToUpdateViaCache(
        String* upateViaCache)
    {
        if (upateViaCache->equals("imports")) {
            return ServiceWorkerUpdateViaCache::Imports;
        } else if (upateViaCache->equals("all")) {
            return ServiceWorkerUpdateViaCache::All;
        } else if (upateViaCache->equals("none")) {
            return ServiceWorkerUpdateViaCache::None;
        } else {
            STARFISH_LOG_WARN("Invalid UpdateViaCache value");
        }

        return Optional<ServiceWorkerUpdateViaCache>();
    }

    static String* updateViaCacheToString(
        ServiceWorkerUpdateViaCache updateViaCache)
    {
        switch (updateViaCache) {
        case ServiceWorkerUpdateViaCache::Imports:
            return String::createASCIIString("imports");
        case ServiceWorkerUpdateViaCache::All:
            return String::createASCIIString("all");
        case ServiceWorkerUpdateViaCache::None:
            return String::createASCIIString("none");
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
};

// NOTE: We consider ExecutionContext as service worker environment (a.k.a
// service worker client, https://w3c.github.io/ServiceWorker/#dfn-service
// -worker-client) In order to distinguish ServiceWorkerClient (https://w3c.
// github.io/ServiceWorker/#client-interface) from it, we use the term,
// ServiceWorkerEnvironment.
using ServiceWorkerEnvironment = ExecutionContext;
using ServiceWorkerRegistrationKey = String*;

using ServiceWorkerJobId = Id<ServiceWorkerJob>;
using ServiceWorkerContextId = Id<GlobalScope>;
using ServiceWorkerClientId = ServiceWorkerContextId;
using ServiceWorkerRegistrationId = Id<ServiceWorkerRegistration>;
using RequestId = Id<ServiceWorkerRequest>;
using PushManagerId = Id<GlobalScope>;

/*
    * SERVICE_WORKER_USE_SEPARATE_PROCESS
    : If true, a service worker runs on a separate process.

    * SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
    : If true, each service worker uses different handles for ipc.

    * STARFISH_WEBWORKER_HOST
    : Code blocks in this scope are only for service worker host.
*/

#define SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION

#ifdef SERVICE_WORKER_USE_SEPARATE_PROCESS
#define IPC_PROTOCOL "ipc://"
#define IPC_ADDRESS_PREFIX "/tmp/.ipc/"
#else
#define IPC_PROTOCOL "inproc://"
#define IPC_ADDRESS_PREFIX "sw/"
#endif
#define IPC_ADDRESS "host"

} // namespace Starfish

#endif
