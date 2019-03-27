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
    !defined(__StarfishServiceWorkerRegistration__)
#define __StarfishServiceWorkerRegistration__

#include "core/dom/EventTarget.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class ServiceWorker;
class ServiceWorkerData;
class String;

enum class ServiceWorkerRegistrationState {
    Installing = 0,
    Waiting,
    Active,
};

class ServiceWorkerRegistrationData : public gc {
public:
    ServiceWorkerRegistrationData();

    DEFINE_GETTER_SETTER(String*, scope, Scope);
    DEFINE_GETTER_SETTER(ServiceWorkerUpdateViaCache, updateViaCache,
                         UpdateViaCache);
    DEFINE_GETTER_SETTER(ServiceWorkerData*, installingWorkerData,
                         InstallingWorkerData);
    DEFINE_GETTER_SETTER(ServiceWorkerData*, waitingWorkerData,
                         WaitingWorkerData);
    DEFINE_GETTER_SETTER(ServiceWorkerData*, activeWorkerData,
                         ActiveWorkerData);

    void updateRegistrationState(ServiceWorkerRegistrationState state,
                                 ServiceWorkerData* serviceWorkerData);

private:
    String* m_scope;
    ServiceWorkerUpdateViaCache m_updateViaCache;
    ServiceWorkerData* m_installingWorkerData;
    ServiceWorkerData* m_waitingWorkerData;
    ServiceWorkerData* m_activeWorkerData;
};

class ServiceWorkerRegistration : public EventTarget {
public:
    ServiceWorkerRegistration(Document* document);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ServiceWorkerRegistration)

    String* scope() const;
    String* updateViaCache() const;
    ServiceWorker* installing() const;
    ServiceWorker* waiting() const;
    ServiceWorker* active() const;

    void updateRegistrationState(ServiceWorkerRegistrationState state,
                                 ServiceWorker* serviceWorker);

    DEFINE_GETTER_SETTER(ServiceWorkerRegistrationData*, data, Data);

private:
    ServiceWorker* m_installingWorker;
    ServiceWorker* m_waitingWorker;
    ServiceWorker* m_activeWorker;

    ServiceWorkerRegistrationData* m_data;
};
}

#endif
