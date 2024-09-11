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

#ifdef STARFISH_ENABLE_WORKER

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/RunLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/worker/WebWorker.h"
#include "core/modules/worker/WorkerThread.h"
#include "core/modules/worker/WorkerGlobalScope.h"

#include "core/modules/worker/WorkerHost.h"

namespace Starfish {

void WorkerHost::run(void* data)
{
    STARFISH_ASSERT(Escargot::Globals::supportsThreading());
    Escargot::Globals::initializeThread();

    RunLoop* runLoop = RunLoop::create();
    WorkerThread* workerThread = static_cast<WorkerThread*>(data);

    WorkerHost host = WorkerHost(workerThread, runLoop);

    if (!workerThread->wasTerminated()) {
        workerThread->onWorkerRunLoopStarted(runLoop);
        runLoop->run();
    }

    host.dispose();

    delete runLoop;

    Escargot::Globals::finalizeThread();
}

WorkerHost::WorkerHost(WorkerThread* workerThread, RunLoop* runLoop)
    : m_wasDisposed(false)
{
    const WorkerHostInitData& initData = workerThread->workerHostInitData();
    m_webWorker =
        new WebWorker(workerThread->starfish(), runLoop,
                      initData.locale.c_str(), initData.timezoneID.c_str(),
                      String::fromUTF8(initData.userAgent.data(),
                                       initData.userAgent.length()));

    m_globalScope = workerThread->createWorkerGlobalScope(m_webWorker, this);
}

void WorkerHost::dispose()
{
    if (m_wasDisposed) {
        return;
    }
    m_wasDisposed = true;

    m_webWorker->timer()->clear(m_globalScope);
    m_webWorker->messageLoop()->clearPendingIdlers(m_globalScope);
    m_globalScope->dispose();
    m_webWorker->destroy();
}

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
