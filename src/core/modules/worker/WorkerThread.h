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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerThread__)
#define __StarfishWorkerThread__

namespace Starfish {

class ExecutionContext;
class Thread;
class Mutex;
class ResourceURL;
class RunLoop;
class WebWorker;
class WorkerGlobalScope;
class Worker;

class WorkerThread : public gc {
public:
    WorkerThread(ExecutionContext* executionContext);

    virtual WorkerGlobalScope* createWorkerGlobalScope(
        WebWorker* webWorker, ResourceURL* scriptURL) = 0;

    void start(Worker* workerObject);

    void terminate();

    void onWorkerRunLoopStarted(RunLoop* runLoop);

    DEFINE_GETTER(RunLoop*, runLoop);
    DEFINE_GETTER(bool, wasWorkerTerminated);

private:
    ExecutionContext* m_executionContext;
    Thread* m_mainThread;
    std::thread m_workerThread;
    Mutex* m_mutex;
    RunLoop* m_runLoop;
    std::atomic_bool m_wasWorkerTerminated;

    static void* workerMainThreadWork(void* data, std::future<void>&& stopTask);

    void destroyWorkerThread();

    bool stopWorkerRunLoop();
};

} // namespace Starfish

#endif
