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
    !defined(__StarfishServiceWorkerJob__)
#define __StarfishServiceWorkerJob__

namespace Starfish {

class ExecutionContext;
class Promise;
class String;
class ServiceWorkerHostJobQueue;

struct ServiceWorkerJob : public gc {
    ServiceWorkerJobId id;
    ServiceWorkerJobType type;
    String* scopeURL;
    String* scriptURL;
    Promise* promise;
    ServiceWorkerClient* client;
    String* referrerURL;
    WorkerType workerType;
    ServiceWorkerUpdateViaCache updateViaCacheMode;
    ServiceWorkerHostJobQueue* containingJobQueue;

    ServiceWorkerRegistrationKey registrationKey();

    ServiceWorkerJob()
        : id(0)
        , type(ServiceWorkerJobType::Register)
        , scopeURL(nullptr)
        , scriptURL(nullptr)
        , promise(nullptr)
        , client(nullptr)
        , referrerURL(nullptr)
        , workerType(WorkerType::Classic)
        , updateViaCacheMode(ServiceWorkerUpdateViaCache::None)
        , containingJobQueue(nullptr)
    {
    }
};

} // namespace Starfish

#endif
