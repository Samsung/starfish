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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishServiceWorkerAgent__
#define __StarfishServiceWorkerAgent__

namespace Starfish {

class WebWorker;
class PerProcess;
class ServiceWorkerData;
class NotificationService;
class ServiceWorkerServer;
class CastServer;

enum class ServiceWorkerAgentState {
    Terminated,
};

using ServiceWorkerAgentStateHandler = void (*)(ServiceWorkerAgentState);

class ServiceWorkerAgent : public gc {
public:
    static ServiceWorkerAgent* create(Starfish* starfish,
                                      PerProcess* perProcess);
    static bool isCreated();
    static ServiceWorkerAgent* instance();

    void destroy();

    void onWebWorkerTerminated(WebWorker* worker);
    void registerOnStatusChangedHandler(ServiceWorkerAgentStateHandler cb);

    void runServiceWorker(ServiceWorkerData* serviceWorker);
    void runServiceWorker(String* scriptURL);
    void abortServiceWorkerScript(ServiceWorkerData* serviceWorker);

    NotificationService* notificationService()
    {
        return m_notificationService;
    }

#if defined(STARFISH_ENABLE_CAST_SERVICE)
    CastServer* castServer()
    {
        return m_castServer;
    }
#endif

private:
    ServiceWorkerAgent(Starfish* starfish, PerProcess* perProcess);
    virtual ~ServiceWorkerAgent();

    static ServiceWorkerAgent* m_instance;
    Starfish* m_starfish;
    PerProcess* perProcess_;
    ServiceWorkerServer* m_SWServer;
    NULLABLE ServiceWorkerAgentStateHandler m_clientFunc{ nullptr };
    NotificationService* m_notificationService;
    GCVector<WebWorker*> m_webWorkerList;
#if defined(STARFISH_ENABLE_CAST_SERVICE)
    CastServer* m_castServer;
#endif
};
} // namespace Starfish
#endif
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
