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

#include "core/modules/threading/ThreadClient.h"
#include "core/modules/worker/WorkerHostInitData.h"

namespace Starfish {

class Starfish;
class MessageLoop;
class GlobalScope;
class Thread;
class Mutex;
class ResourceURL;
class RunLoop;
class WebBase;
class WebWorker;
class WorkerGlobalScope;
class WorkerHost;

class WorkerThreadClient : public ThreadClient, public gc {
public:
    using ThreadFinishedCallback = void (*)(void* data);

    void onThreadStarted(Thread* thread) override;
    void onThreadFinished(Thread* thread) override;

    void setThreadFinishedCallback(ThreadFinishedCallback callback, void* data);

private:
    void* m_threadFinishedCallbackData{ nullptr };
    ThreadFinishedCallback m_threadFinishedCallback{ nullptr };
};

class WorkerThread : public gc {
public:
    enum class State { None, Running, Terminated };

    virtual WorkerGlobalScope* createWorkerGlobalScope(
        WebWorker* webWorker, WorkerHost* workerHost) = 0;

    void start();

    void terminate();

    void setOnTerminatedCallback(
        WorkerThreadClient::ThreadFinishedCallback callback, void* data);

    void onWorkerRunLoopStarted(RunLoop* runLoop);

    void addChildThread(WorkerThread* thread);
    void removeChildThread(WorkerThread* thread);

    bool isRunning();
    bool wasTerminated();

    ResourceURL* createScriptURL();

    DEFINE_GETTER(Starfish*, starfish);
    DEFINE_GETTER(MessageLoop*, messageLoop);
    DEFINE_GETTER(RunLoop*, runLoop);
    DEFINE_GETTER(GlobalScope*, workerMessageLoopGlobalScope);
    DEFINE_GETTER(const WorkerHostInitData&, workerHostInitData);

protected:
    static void* workerMainThreadWork(void* data, std::future<void>&& stopTask);

    WorkerThread(Starfish* starfish, MessageLoop* messageLoop,
                 const WorkerHostInitData& initData);

    WorkerThread(WebBase* webBase, ResourceURL* scriptURL);

    virtual ~WorkerThread();

    void initializeWorkerThread();

    void destroyWorkerThread();

    bool stopWorkerRunLoop();

    void terminateChildThreads();

    Starfish* m_starfish;
    MessageLoop* m_messageLoop;
    Thread* m_mainThread;
    WorkerThreadClient* m_mainThreadClient;
    std::thread m_workerThread;
    Mutex* m_mutex;
    RunLoop* m_runLoop;
    std::atomic<State> m_state;
    GlobalScope* m_workerMessageLoopGlobalScope;
    Mutex* m_childThreadDataLock;
    std::vector<WorkerThread*> m_childThreads;
    WorkerHostInitData m_workerHostInitData;

private:
    WorkerThread(Starfish* starfish, MessageLoop* messageLoop);
};

} // namespace Starfish

#endif
