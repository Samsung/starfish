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
    !defined(__StarfishServiceWorkerJob__)
#define __StarfishServiceWorkerJob__

namespace Starfish {

class ExecutionContext;
class Promise;
class String;
class JobQueue;
class IServiceWorkerHostConnection;
class IServiceWorkerClientConnection;
class ServiceWorkerJobData;

class Job : public gc {
};

class ServiceWorkerJob : public Job {
public:
    ServiceWorkerJob(ServiceWorkerJobData* data = nullptr);
    ServiceWorkerRegistrationKey registrationKey();

    DEFINE_GETTER_SETTER(Promise*, promise, Promise);
    DEFINE_GETTER_SETTER(ServiceWorkerJobData*, data, Data);
    DEFINE_GETTER_SETTER(ServiceWorkerEnvironment*, client, Client);
    DEFINE_GETTER_SETTER(JobQueue*, containingJobQueue, ContainingJobQueue);
    DEFINE_GETTER_SETTER(IServiceWorkerClientConnection*, hostConnection,
                         HostConnection);
    DEFINE_GETTER_SETTER(IServiceWorkerHostConnection*, clientConnection,
                         ClientConnection);

private:
    Promise* m_promise{ nullptr };
    ServiceWorkerJobData* m_data{ nullptr };
    ServiceWorkerEnvironment* m_client{ nullptr };
    JobQueue* m_containingJobQueue{ nullptr };
    IServiceWorkerClientConnection* m_hostConnection{ nullptr };
    IServiceWorkerHostConnection* m_clientConnection{ nullptr };
};

} // namespace Starfish

#endif
