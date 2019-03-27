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
    !defined(__StarfishServiceWorkerProcessManager__)
#define __StarfishServiceWorkerProcessManager__

namespace Starfish {

class ProcessHost;
class ServiceWorkerClientConnection;

class ServiceWorkerHostProcess;
class ServiceWorkerClientProcess;

class ThreadPool;
class IThread;
class IORunnable;

struct ProcessData {
    ProcessData()
        : pid(-1)
        , connection(nullptr)
    {
    }
    PID pid;
    std::string origin;
    std::string connectionAddress;
    ServiceWorkerClientConnection* connection;
};

class ServiceWorkerProcessManager : public gc {
public:
    static ServiceWorkerProcessManager* getInstance();
    static void destroy();

    void init(ThreadPool* threadPool);
    ServiceWorkerClientConnection* getConnection(std::string origin);

private:
    ServiceWorkerProcessManager();
    virtual ~ServiceWorkerProcessManager();
    static ServiceWorkerProcessManager* m_instance;

    IThread* m_ioThread;
    ThreadPool* m_threadPool;
    IORunnable* m_ioRunnable;

    ServiceWorkerHostProcess* m_serviceWorkerHostProcess;
    ServiceWorkerClientProcess* m_serviceWorkerClientProcess;

    std::unordered_map<std::string, std::shared_ptr<ProcessData>>
        m_mapOriginToProcessData;
};
} // namespace Starfish

#endif
