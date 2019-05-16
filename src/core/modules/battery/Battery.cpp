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
#ifdef STARFISH_ENABLE_BATTERY_STATUS

#include "StarfishConfig.h"
#include "Starfish.h"
#include "Battery.h"

#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
#include <device/battery.h>
#include <device/callback.h>
#endif

namespace Starfish {

BatteryManager::BatteryManager(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

BatteryManager::~BatteryManager()
{
}

ExecutionContext* BatteryManager::executionContext() const
{
    return m_executionContext;
}

double BatteryManager::level()
{
#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
    int batteryLevel = 0;
    int ret = device_battery_get_percent(&batteryLevel);
    if (ret == DEVICE_ERROR_NONE) {
        return ((double)batteryLevel) / 100;
    } else {
        STARFISH_ASSERT(0);
    }
#endif
    return 0.0;
}

} // namespace Starfish

#endif
