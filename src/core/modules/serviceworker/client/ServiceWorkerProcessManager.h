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
    !defined(__StarfishServiceWorkerProcessManager__)
#define __StarfishServiceWorkerProcessManager__

namespace Starfish {

class ProcessHost;
class ServiceWorkerClientConnection;
class PerProcess;
class GlobalScope;
class ServiceWorkerAgent;
class PushServiceAgent;

struct ProcessData {
    ProcessData()
        : pid(-1)
        , connection(nullptr)
    {
    }
    PID pid;
    std::string origin;
    std::string connectionAddress;
    ServiceWorkerClientConnection* connection{ nullptr };
};

class ServiceWorkerProcessManager : public gc {
public:
    static ServiceWorkerProcessManager* instance();
    static std::string createAddress(const std::string& lastAddress = "");

    void init(PerProcess* perProcess);
    void destroy();

    ServiceWorkerClientConnection* getConnection(String* originSerialized);

    void registerActiveGlobalScope(Id<GlobalScope> id,
                                   GlobalScope* globalScope);
    void deregisterActiveGlobalScope(Id<GlobalScope> id);
    NULLABLE GlobalScope* findGlobalScope(Id<GlobalScope> id);

    PushServiceAgent* pushServiceAgent()
    {
        return m_pushServiceAgent;
    }

private:
    ServiceWorkerProcessManager() = default;
    ~ServiceWorkerProcessManager() = default;

    static ServiceWorkerProcessManager* m_instance;

    PerProcess* perProcess_{ nullptr };
    PushServiceAgent* m_pushServiceAgent{ nullptr };
    ServiceWorkerClientConnection* m_connection{ nullptr };

#if !defined(SERVICE_WORKER_USE_SEPERATED_PROCESS)
    ServiceWorkerAgent* m_agent{ nullptr };
#endif

    std::unordered_map<std::string, std::shared_ptr<ProcessData>>
        m_mapOriginToProcessData;

    GCUnorderedMap<Id<GlobalScope>, GlobalScope*, IdHash>
        m_mapIdToActiveGlobalScope;
};
} // namespace Starfish

#endif
