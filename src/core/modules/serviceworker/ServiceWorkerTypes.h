/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

namespace Starfish {

class GlobalScope;
class ServiceWorkerJob;
class ExecutionContext;
class ServiceWorkerRequest;
class ServiceWorkerRegistration;

enum class ServiceWorkerState : unsigned {
    Installing = 0,
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

enum class ServiceWorkerUpdateViaCache : unsigned {
    Imports,
    All,
    None,
};

enum class ServiceWorkerJobType : unsigned {
    Register,
    Unregister,
    Update,
};

enum class WorkerType : unsigned {
    Classic,
    Module,
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
using ServiceWorkerRegistrationId = Id<ServiceWorkerRegistration>;
using ServiceWorkerClientId = ServiceWorkerContextId;
using RequestId = Id<ServiceWorkerRequest>;

#ifdef SERVICE_WORKER_USE_MULTI_PROCESS
#define IPC_PROTOCOL "ipc://"
#define IPC_ADDRESS_PREFIX ".ipc/"
#else
#define IPC_PROTOCOL "inproc://"
#define IPC_ADDRESS_PREFIX "sw/"
#endif
} // namespace Starfish

#endif
