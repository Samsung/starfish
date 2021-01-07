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
    !defined(__StarfishServiceWorkerHostJobQueue__)
#define __StarfishServiceWorkerHostJobQueue__

namespace Starfish {

class ServiceWorkerJob;
class MessageLoop;
class ServiceWorkerRegistrationData;
class ServiceWorker;
class ServiceWorkerData;

class JobQueue : public gc {
public:
    JobQueue();

    void enqueueJob(ServiceWorkerJob* job);
    void dequeueJob();
    size_t size() const;
    bool empty() const;
    ServiceWorkerJob* firstJob() const;
    ServiceWorkerJob* lastJob() const;

private:
    // TODO: this should have an identifier.
    GCDeque<ServiceWorkerJob*> m_jobQueue;
};
} // namespace Starfish

#endif
