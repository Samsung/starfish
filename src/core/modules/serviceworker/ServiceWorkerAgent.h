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
#ifndef __StarfishServiceWorkerAgent__
#define __StarfishServiceWorkerAgent__

#include "core/modules/serviceworker/notification/NotificationOptions.h"

namespace Starfish {

class ServiceWorkerAgent : public gc {
public:
    static ServiceWorkerAgent* instance();

    void appendNotification(NotificationOptions& options);

    bool replaceNotification(NotificationOptions& options);

private:
    ServiceWorkerAgent() = default;
    virtual ~ServiceWorkerAgent() = default;

    static ServiceWorkerAgent* m_instance;
    GCVector<NotificationOptions> m_notificationList;
};
}
#endif
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
