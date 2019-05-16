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

#if defined(STARFISH_ENABLE_BATTERY_STATUS) && !defined(__StarfishBattery__)
#define __StarfishBattery__

#include "core/dom/EventTarget.h"
#include "core/dom/ExecutionContext.h"
#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace Starfish {

class BatteryManager : public EventTarget {
public:
    BatteryManager(ExecutionContext* executionContext);
    virtual ~BatteryManager();

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isBatteryManager() const override;
    virtual ExecutionContext* executionContext() const override;

    virtual double level();

protected:
    ExecutionContext* m_executionContext;
};

} // namespace Starfish

#endif
