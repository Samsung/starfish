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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishPushManager__
#define __StarfishPushManager__

#include "binding/ScriptWrappable.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/push/PushSubscriptionOptions.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class PushSubscription;

class PushManager : public ScriptWrappable {
public:
    PushManager(ExecutionContext* executionContext,
                ServiceWorkerRegistration* registration);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PushManager)

    Promise* subscribe();
    Promise* subscribe(PushSubscriptionOptionsInit& options);

    Promise* getSubscription();

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }
    ServiceWorkerRegistration* registration()
    {
        return m_registration;
    }

    NULLABLE PushSubscription* pushSubscription()
    {
        return m_pushSubscription;
    }

    void setPushSubscription(PushSubscription* pushSubscription);

    PushManagerId& pushManagerId()
    {
        return m_pushManagerId;
    }

    PushSubscriptionOptionsInit& optionsInit()
    {
        return m_optionsInit;
    }

private:
    ExecutionContext* m_executionContext;
    ServiceWorkerRegistration* m_registration;
    String* m_applicationServerKey;
    NULLABLE PushSubscription* m_pushSubscription;
    PushSubscriptionOptionsInit m_optionsInit;
    PushManagerId m_pushManagerId;
};
}

#endif
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
