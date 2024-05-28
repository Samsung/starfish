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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerHostProxy__)
#define __StarfishWorkerHostProxy__

#include "core/modules/worker/WorkerProxy.h"

namespace Starfish {

class ExecutionContext;
class MessageLoop;
class SerializeWithTransferResult;
class WorkerThread;
class WorkerHost;
class Mutex;

class WorkerHostProxy final : public WorkerProxy {
public:
    WorkerHostProxy(ExecutionContext* executionContext,
                    WorkerThread* workerThread);

    bool initialize(WorkerHost* workerHost);

    void onScriptLoadFinished();

    void terminateWorkerGlobalScope();

    DEFINE_GETTER(WorkerThread*, workerThread);
    DEFINE_GETTER(bool, askedToTerminate);

private:
    WorkerHost* m_workerHost;
    bool m_wasWorkerScriptLoaded;
    bool m_askedToTerminate;
    GCVector<SerializeWithTransferResult*> m_queuedEarlyMessages;
    Mutex* m_mutex;

    MessageLoop* targetMessageLoop() override;
    ExecutionContext* targetExecutionContext() override;

    void postSerializedMessage(
        SerializeWithTransferResult* serializedMessage) override;

    void handleQueuedEarlyMessages();
};

} // namespace Starfish

#endif
