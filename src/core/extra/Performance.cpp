/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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
#include "binding/ScriptWrappable.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/ExecutionContext.h"
#include "core/extra/Performance.h"

namespace Starfish {

Performance::Performance(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
}

double Performance::now()
{
    return (longTickCount() - m_executionContext->createdTick()) / 1000.0;
}
} // namespace Starfish
