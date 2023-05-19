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

#include "StarfishBase.h"
#include "platform/process/base/ProcessType.h" // PID
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/util/Id.h"

#include <future>

namespace Starfish {

class ProcessHost;
class ServiceWorkerClientConnection;
class PerProcess;
class GlobalScope;
class ServiceWorkerAgent;
class PushServiceAgent;
class ServiceWorkerFetchTask;
class FetchEventHandler;
class String;
class ServiceWorkerOption;
class RegistrationManager;

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

    void init(PerProcess* perProcess, ServiceWorkerOption* option);
    void destroy();

    ServiceWorkerClientConnection* getConnection(String* originSerialized);
    bool startWorkerOnThread(std::string scriptURL);
    void registerActiveGlobalScope(Id<GlobalScope> id,
                                   GlobalScope* globalScope);
    void deregisterActiveGlobalScope(Id<GlobalScope> id);
    NULLABLE GlobalScope* findGlobalScope(Id<GlobalScope> id);
    const GCVector<ServiceWorkerEnvironment*>& getSettingsObjects(
        String* scriptURL);

    PushServiceAgent* pushServiceAgent()
    {
        return m_pushServiceAgent;
    }

    RegistrationManager* registrationManager()
    {
        return m_registrationManager;
    }

    Nullable<FetchEventHandler*> findFetchEventHandler(Id<GlobalScope> id);

private:
    ServiceWorkerProcessManager() = default;
    ~ServiceWorkerProcessManager() = default;

    bool processExist(const std::string identifier);

    static ServiceWorkerProcessManager* m_instance;

    PerProcess* m_perProcess{ nullptr };
    ServiceWorkerOption* m_option{ nullptr };
    PushServiceAgent* m_pushServiceAgent{ nullptr };
    ServiceWorkerClientConnection* m_connection{ nullptr };
    RegistrationManager* m_registrationManager{ nullptr };

    std::unordered_map<std::string, std::shared_ptr<ProcessData>>
        m_mapOriginToProcessData;
    // TODO: replace m_mapIdToActiveGlobalScope with m_settingsObjects.
    GCUnorderedMap<Id<GlobalScope>, GlobalScope*, IdHash>
        m_mapIdToActiveGlobalScope;
    GCUnorderedMap<Id<GlobalScope>, FetchEventHandler*, IdHash>
        m_fetchEventHandlers;

    GCVector<ServiceWorkerEnvironment*> m_settingsObjects;
    bool m_settingsObjectsNeedUpdated{ true };

#if !defined(SERVICE_WORKER_USE_SEPARATE_PROCESS)
    std::promise<void> m_promiseStopThreadSignal;
#endif
};
} // namespace Starfish

#endif
