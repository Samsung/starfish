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
#ifdef STARFISH_ENABLE_BATTERY_STATUS

#include "StarfishConfig.h"
#include "Starfish.h"
#include "Battery.h"

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
    return 1.0;
}

} // namespace Starfish

#endif
