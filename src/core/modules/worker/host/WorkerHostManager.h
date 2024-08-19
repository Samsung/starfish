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

#ifndef __StarfishWorkerHostManager__
#define __StarfishWorkerHostManager__

#include "core/modules/worker/WorkerManager.h"

namespace Starfish {

class ServiceWorkerProcessManager;

class WorkerHostManager : public WorkerManager {
    friend class WorkerManager;

public:
    bool isWorkerHostManager() const override
    {
        return true;
    }

private:
    WorkerHostManager(Starfish* starfish);

    static const size_t s_threadPoolSize = 5;
};

} // namespace Starfish

#endif
#endif
