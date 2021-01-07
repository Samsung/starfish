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
#ifndef __StarfishPushServiceAgent__
#define __StarfishPushServiceAgent__

#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

enum class PushPermissionState { Denied, Granted, Prompt };

class PushSubscription;
struct PushSubscriptionOptionsInit;

class PushServiceAgent : public gc {
public:
    PushServiceAgent();

    bool requestSubscripbe(PushManagerId id,
                           NULLABLE PushSubscription*& pushSubscription,
                           PushSubscriptionOptionsInit& optionsInit,
                           ExecutionContext* executionContext);
    NULLABLE PushSubscription* findSubscription(PushManagerId& id);

private:
    PushPermissionState m_permissionState;
    GCUnorderedMap<PushManagerId, PushSubscription*, IdHash> m_SubscriptionMap;
};
} // namespace Starfish

#endif
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
