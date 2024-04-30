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

#if defined(STARFISH_ENABLE_WORKER)
#ifndef __StarfishDedicatedWorkerThread__
#define __StarfishDedicatedWorkerThread__

#include "core/modules/worker/WorkerThread.h"

namespace Starfish {

class WebBase;
class Worker;
class WebWorker;
class WorkerHost;

class DedicatedWorkerThread final : public WorkerThread {
public:
    DedicatedWorkerThread(WebBase* webBase, Worker* worker);
    ~DedicatedWorkerThread();

    WorkerGlobalScope* createWorkerGlobalScope(WebWorker* webWorker,
                                               WorkerHost* workerHost) override;

    DEFINE_GETTER(Worker*, worker);

private:
    Worker* m_worker;
};

} // namespace Starfish

#endif
#endif
