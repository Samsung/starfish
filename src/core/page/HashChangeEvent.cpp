/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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
#include "core/page/HashChangeEvent.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

HashChangeEventInit::HashChangeEventInit()
    : EventInit(false, false)
    , m_oldURL(String::emptyString)
    , m_newURL(String::emptyString)
{
}

HashChangeEvent::HashChangeEvent(ExecutionContext* executionContext)
    : Event(executionContext)
    , m_oldURL(String::emptyString)
    , m_newURL(String::emptyString)

{
}

HashChangeEvent::HashChangeEvent(ExecutionContext* executionContext,
                                 String* type)
    : Event(executionContext, type)
    , m_oldURL(String::emptyString)
    , m_newURL(String::emptyString)
{
}

HashChangeEvent::HashChangeEvent(ExecutionContext* executionContext,
                                 String* type,
                                 const HashChangeEventInit& hashChangeEventInit)
    : Event(executionContext, type, hashChangeEventInit)
    , m_oldURL(hashChangeEventInit.oldURL())
    , m_newURL(hashChangeEventInit.newURL())
{
}

} // namespace Starfish
