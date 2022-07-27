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
    !defined(__StarfishServiceWorkerRegistrationData__)
#define __StarfishServiceWorkerRegistrationData__

#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/ServiceWorkerUpdateViaCache.h"
#include <string>

namespace Starfish {

class String;
class ServiceWorkerData;

class ServiceWorkerRegistrationData : public Archivable {
public:
    ServiceWorkerRegistrationId id;
    String* scope{ String::emptyString };

    DEFINE_GETTER(Nullable<ServiceWorkerData*>, installingWorker);
    DEFINE_GETTER(Nullable<ServiceWorkerData*>, waitingWorker);
    DEFINE_GETTER(Nullable<ServiceWorkerData*>, activeWorker);
    void setInstallingWorker(Nullable<ServiceWorkerData*> worker);
    void setWaitingWorker(Nullable<ServiceWorkerData*> worker);
    void setActiveWorker(Nullable<ServiceWorkerData*> worker);

    ServiceWorkerUpdateViaCache updateViaCache{
        ServiceWorkerUpdateViaCache::None
    };

    bool isValid()
    {
        return (scope != String::emptyString);
    }

    DEFINE_GETTER_SETTER(bool, isUninstalling, IsUninstalling);

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;

private:
    Nullable<ServiceWorkerData*> m_waitingWorker;
    Nullable<ServiceWorkerData*> m_activeWorker;
    Nullable<ServiceWorkerData*> m_installingWorker;

    void sendEventTask(std::string eventname);

    bool m_isUninstalling{ false };
};

} // namespace Starfish

#endif
