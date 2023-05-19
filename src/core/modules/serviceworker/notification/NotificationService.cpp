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

#ifdef STARFISH_ENABLE_SERVICE_WORKER_NOTIFICATION

#include "StarfishConfig.h"

#include "core/modules/serviceworker/notification/NotificationService.h"

namespace Starfish {

void NotificationService::appendNotification(NotificationOptions& options)
{
    m_notificationList.push_back(options);
}

bool NotificationService::replaceNotification(
    NotificationOptions& currentOption)
{
    if (currentOption.tag()->isEmpty() == true) {
        return false;
    }

    auto iter = m_notificationList.begin();
    while (iter != m_notificationList.end()) {
        auto& option = *iter;
        if (option.origin()->equals(currentOption.origin()) == true) {
            *iter = currentOption;
            return true;
        }

        iter++;
    }
    return false;
}
} // namespace Starfish
#endif /* STARFISH_ENABLE_SERVICE_WORKER_NOTIFICATION */
