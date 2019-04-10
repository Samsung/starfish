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
#include "core/util/Id.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"

namespace Starfish {

class ServiceWorker;
class ServiceWorkerData;
class String;

class ServiceWorkerRegistration : public EventTarget {
public:
    ServiceWorkerRegistration(ExecutionContext* executionContext);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isServiceWorkerRegistration() const override;

    virtual ExecutionContext* executionContext() override;

    String* scope() const;             // binding interface
    String* updateViaCache() const;    // binding interface
    ServiceWorker* installing() const; // binding interface
    ServiceWorker* waiting() const;    // binding interface
    ServiceWorker* active() const;     // binding interface

    void updateRegistrationState(ServiceWorkerRegistrationState state,
                                 ServiceWorker* serviceWorker);

    DEFINE_GETTER_SETTER(ServiceWorkerRegistrationData*, data, Data);

private:
    ExecutionContext* m_executionContext;
    ServiceWorker* m_installingWorker;
    ServiceWorker* m_waitingWorker;
    ServiceWorker* m_activeWorker;

    ServiceWorkerRegistrationData* m_data;
};
} // namespace Starfish

#endif
