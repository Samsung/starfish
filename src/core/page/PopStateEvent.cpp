/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/page/PopStateEvent.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

PopStateEventInit::PopStateEventInit()
    : EventInit(false, false)
    , m_state(nullptr)
{
}

PopStateEvent::PopStateEvent(ExecutionContext* executionContext)
    : Event(executionContext)
    , m_state(nullptr)

{
}

PopStateEvent::PopStateEvent(ExecutionContext* executionContext, String* type)
    : Event(executionContext, type)
    , m_state(nullptr)
{
}

PopStateEvent::PopStateEvent(ExecutionContext* executionContext, String* type,
                             const PopStateEventInit& popStateEventInit)
    : Event(executionContext, type, popStateEventInit)
    , m_state(popStateEventInit.state())
{
}

} // namespace Starfish
