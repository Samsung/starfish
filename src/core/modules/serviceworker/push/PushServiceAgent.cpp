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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"
#include "core/util/Id.h"
#include "core/modules/serviceworker/push/PushSubscription.h"
#include "core/modules/serviceworker/push/PushSubscriptionOptions.h"
#include "core/modules/serviceworker/push/PushServiceAgent.h"
namespace Starfish {

PushServiceAgent::PushServiceAgent()
    : m_permissionState(PushPermissionState::Granted)
{
}

bool PushServiceAgent::requestSubscripbe(
    PushManagerId id, NULLABLE PushSubscription*& pushSubscription,
    PushSubscriptionOptionsInit& optionsInit,
    ExecutionContext* executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);

    if (m_permissionState != PushPermissionState::Granted) {
        return false;
    }

    pushSubscription = findSubscription(id);
    if (pushSubscription != nullptr) {
        return false;
    }

    // TODO: Run push service and set subscription options
    pushSubscription = new PushSubscription(executionContext);
    pushSubscription->options()->setPushSubscriptionOptions(optionsInit);
    return true;
}

NULLABLE PushSubscription* PushServiceAgent::findSubscription(PushManagerId& id)
{
    auto it = m_SubscriptionMap.find(id);
    if (it == m_SubscriptionMap.end()) {
        return nullptr;
    }
    return it->second;
}
}

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
