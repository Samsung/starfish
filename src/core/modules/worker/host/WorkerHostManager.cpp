/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_USE_WORKER_PROCESS) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"

#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/modules/worker/util/LocalStorageHelper.h"
#include "core/modules/worker/host/WorkerHostManager.h"

namespace Starfish {

WorkerHostManager::WorkerHostManager(Starfish* starfish)
    : WorkerManager(starfish)
{
    m_workerSettings->setThreadPoolSize(s_threadPoolSize);
}

} // namespace Starfish

#endif /* STARFISH_USE_WORKER_PROCESS */
