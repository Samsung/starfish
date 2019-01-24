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

#include "core/page/ScriptExecutionContext.h"

namespace Starfish {

#ifdef STARFISH_ENABLE_SERVICE_WORKER

ServiceWorker* ScriptExecutionContext::activeServiceWorker() const
{
    return m_activeServiceWorker;
};

void ScriptExecutionContext::setActiveServiceWorker(
    ServiceWorker* serviceWorker)
{
    m_activeServiceWorker = serviceWorker;
}

#endif // STARFISH_ENABLE_SERVICE_WORKER
}
