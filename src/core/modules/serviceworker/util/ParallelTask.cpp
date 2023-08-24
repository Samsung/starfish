/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/PerProcess.h"
#include "core/modules/threading/ThreadPool.h"

namespace Starfish {

IdleTask::IdleTask(GlobalScope* globalScope)
    : m_globalScope(globalScope)
{
}

void IdleTask::start()
{
    IdleTask::queue(this);
}

void IdleTask::queue(IdleTask* task)
{
    task->globalScope()->webBase()->messageLoop()->addIdler(
        task->globalScope(),
        [](size_t handle, void* data) {
            IdleTask* task = static_cast<IdleTask*>(data);
            task->run();
            task->end();
        },
        task);
}

#if defined(STARFISH_WEBWORKER_HOST)

void ParallelTask::start()
{
    ParallelTask::queue(this);
}

void ParallelTask::queue(ParallelTask* task)
{
    PerProcess* perProcess = ServiceWorkerAgent::instance()->perProcess();
    STARFISH_ASSERT(perProcess != nullptr);

    struct Param : public gc {
        Param(PerProcess* p, ParallelTask* t)
            : perProcess(p)
            , task(t)
        {
        }
        PerProcess* perProcess;
        ParallelTask* task;
    };

    // NOTE: Passing a GCed pointer between threads isn't long-term tested.
    Param* param = new Param(perProcess, task);

    // Enqueue a thread task
    perProcess->threadPool()->addWork(
        nullptr,
        [](void* param) -> void* {
            PerProcess* perProcess = static_cast<Param*>(param)->perProcess;
            ParallelTask* task = static_cast<Param*>(param)->task;

            // Run a thread task
            task->run();

            // Enqueue an end handler running on main thread
            perProcess->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t, void* data) {
                    static_cast<ParallelTask*>(data)->end();
                },
                task);

            return nullptr;
        },
        param);
}
#endif
} // namespace Starfish

#endif
