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
    !defined(__StarfishServiceWorkerHostProcess__)
#define __StarfishServiceWorkerHostProcess__

namespace Starfish {

/*
    TODO: generate ServiceWorkerRegistrationKey
    an ordered map where the keys are scope urls, serialized,
    and the values are service worker registrations.
*/

class JobQueue;
class IThread;
class ThreadPool;
class IORunnable;
class MessageLoop;
class ServiceWorkerJob;
class ServiceWorkerHostJobHandler;
class ServiceWorkerHostConnection;
class ServiceWorkerRegistrationData;
class ServiceWorkerClientProcessInterface;
class ProgramOptions;

class ServiceWorkerHostProcess : public gc,
                                 public ServiceWorkerHostProcessInterface {
public:
    static ServiceWorkerHostProcess* getInstance();
    static void destroy();

    ServiceWorkerHostProcess(ServiceWorkerHostProcess const&) = delete;
    void operator=(ServiceWorkerHostProcess const&) = delete;

    void init(ThreadPool* threadPool);
    void start(ProgramOptions* programOptions);

    void scheduleJob(ServiceWorkerJob* job) override;
    void matchRegistration(ServiceWorkerRequest* request,
                           String* clientURL) override;

    DEFINE_GETTER(ServiceWorkerHostConnection*, connection);

private:
    static ServiceWorkerHostProcess* m_instance;

    ServiceWorkerHostProcess();
    virtual ~ServiceWorkerHostProcess();

    MessageLoop* m_messageLoop{ nullptr };
    IThread* m_ioThread{ nullptr };
    ThreadPool* m_threadPool{ nullptr };
    IORunnable* m_ioRunnable{ nullptr };
    ServiceWorkerHostJobHandler* m_jobHandler{ nullptr };
    ServiceWorkerHostConnection* m_connection{ nullptr };
    ProgramOptions* m_po{ nullptr };
};

} // namespace Starfish
#endif
