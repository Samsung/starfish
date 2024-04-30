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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerObjectProxy__)
#define __StarfishWorkerObjectProxy__

#include "core/modules/worker/WorkerProxy.h"

namespace Starfish {

class ExecutionContext;
class WorkerThread;
class MessageLoop;
class Worker;

class WorkerObjectProxy final : public WorkerProxy {
public:
    WorkerObjectProxy(ExecutionContext* executionContext, Worker* worker);

    void terminate() override;

    String* workerName() const;

    void terminateWorker();

    Nullable<WorkerGlobalScope*> parentWorkerGlobalScope();
    void addChildWorker();
    void removeChildWorker();

    DEFINE_GETTER(Worker*, workerObject);

private:
    Worker* m_workerObject;

    MessageLoop* targetMessageLoop() override;
    ExecutionContext* targetExecutionContext() override;
    bool isTargetClosed() override;

    void postSerializedMessage(
        SerializeWithTransferResult* serializedMessage) override;
};

} // namespace Starfish

#endif
