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

#if defined(STARFISH_ENABLE_SHARED_WORKER) || \
    defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "LWEWorker.h"
#include "public/delegate/LWEDelegate.h"
#include "public/delegate/ThreadedCallHelper.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/WorkerManager.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/modules/worker/host/WorkerAgent.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

using namespace Starfish;

namespace LWEDelegate {
extern Starfish::Starfish *g_starfishInstance;
}

namespace LWE {

#if defined(STARFISH_WEBWORKER_HOST)

static WorkerProcessState ToWorkerState(WorkerAgentState state)
{
    switch (state) {
    case WorkerAgentState::Terminated:
        return WorkerProcessState::Terminated;
    default:
        return WorkerProcessState::None;
    }
}

static void initializeWorkerProcess(const std::string &dataDirectoryPath,
                                    const std::string &resourceDirectoryPath)
{
    STARFISH_LOG_INFO("WORKER STARTS");

    std::string workerDataDirPath = dataDirectoryPath;
    if (workerDataDirPath.empty()) {
        workerDataDirPath =
            Starfish::WorkerSettings::getDefaultDataDirectoryPath();
    }

    // TODO: use StoragePathProvider class to get the path of worker directory.
    std::string workerResourceDirPath =
        dataDirectoryPath + resourceDirectoryPath;
    LWEDelegate::LWE::Initialize(workerResourceDirPath.data());

    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [workerDataDirPath]() -> void {
            if (WorkerAgent::isCreated() == false) {
                LWEDelegate::g_starfishInstance->workerManager()
                    ->workerSettings()
                    ->setDataDirectoryPath(workerDataDirPath);

                WorkerAgent *agent =
                    WorkerAgent::create(LWEDelegate::g_starfishInstance);
                agent->start();
            }
        });
}

static void registerOnStatusChangedHandler(
    const std::function<void(WorkerProcessState)> &cb)
{
    STARFISH_RELEASE_ASSERT(WorkerAgent::isCreated());
    auto onStateChangedCallback = [cb](WorkerAgentState state) {
        cb(ToWorkerState(state));
    };

    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void {
            STARFISH_ASSERT(cb != nullptr);
            WorkerAgent::instance()->registerOnStatusChangedHandler(
                onStateChangedCallback);
        });
}

static void finalizeWorkerProcess()
{
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        []() -> void {
            if (WorkerAgent::isCreated() == true) {
                WorkerAgent::instance()->destroy();
            }
        });

    LWEDelegate::LWE::Finalize();

    STARFISH_LOG_INFO("WORKER ENDS");
}

void ServiceWorker::Initialize(const std::string &dataDirectoryPath)
{
    return initializeWorkerProcess(dataDirectoryPath,
                                   "/service-worker-resource");
}

void ServiceWorker::RegisterOnStatusChangedHandler(
    const std::function<void(WorkerProcessState)> &cb)
{
    return registerOnStatusChangedHandler(cb);
}

void ServiceWorker::Finalize()
{
    return finalizeWorkerProcess();
}

void SharedWorker::Initialize(const std::string &dataDirectoryPath)
{
    return initializeWorkerProcess(dataDirectoryPath,
                                   "/shared-worker-resource");
}

void SharedWorker::RegisterOnStatusChangedHandler(
    const std::function<void(WorkerProcessState)> &cb)
{
    return registerOnStatusChangedHandler(cb);
}

void SharedWorker::Finalize()
{
    return finalizeWorkerProcess();
}

#else // !defined(STARFISH_WEBWORKER_HOST)
void WorkerClient::RegisterDataDirectoryPath(
    const std::string &dataDirectoryPath)
{
    STARFISH_RELEASE_ASSERT(LWEDelegate::LWE::IsInitialized());
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [dataDirectoryPath]() -> void {
            LWEDelegate::g_starfishInstance->workerManager()
                ->workerSettings()
                ->setDataDirectoryPath(dataDirectoryPath);
        });
}

void WorkerClient::RegisterServiceWorkerProcessExecutor(
    const std::function<bool()> &fn)
{
    STARFISH_RELEASE_ASSERT(LWEDelegate::LWE::IsInitialized());
    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void {
            LWEDelegate::g_starfishInstance->workerManager()
                ->workerSettings()
                ->setServiceWorkerProcessExecutor(fn);
        });
}

#endif // defined(STARFISH_WEBWORKER_HOST)

} // namespace LWE

#endif
