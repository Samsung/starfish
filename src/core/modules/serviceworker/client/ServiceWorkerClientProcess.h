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
    !defined(__StarfishServiceWorkerClientProcess__)
#define __StarfishServiceWorkerClientProcess__

namespace Starfish {

class ServiceWorkerJob;
class ServiceWorkerJobClient;
class ServiceWorkerRegistrationData;

class ServiceWorkerClientProcess : public ServiceWorkerClientProcessInterface,
                                   public gc {
public:
    static ServiceWorkerClientProcess* getInstance();
    static void destroy();

    ServiceWorkerClientProcess(ServiceWorkerClientProcess const&) = delete;
    void operator=(ServiceWorkerClientProcess const&) = delete;

    void init(ServiceWorkerJobClient* jobClient);
    ServiceWorkerHostProcessInterface* host();

    // Inteface overrided
    void resolveJobPromise(
        ServiceWorkerJob* job,
        ServiceWorkerRegistrationData* registration) override;

private:
    static ServiceWorkerClientProcess* m_instance;
    ServiceWorkerClientProcess();
    virtual ~ServiceWorkerClientProcess();

    ServiceWorkerJobClient* m_jobClient;
};

} // namespace Starfish
#endif
