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

    std::string workerResourceDirPath =
        dataDirectoryPath + resourceDirectoryPath;
    LWEDelegate::LWE::Initialize(
        (workerResourceDirPath + "/starfish-local-storage.txt").c_str(),
        (workerResourceDirPath + "/starfish-cookie.txt").c_str(),
        (workerResourceDirPath + "/starfish-http-cache").c_str());

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([workerDataDirPath]() -> size_t {
        if (WorkerAgent::isCreated() == false) {
            LWEDelegate::g_starfishInstance->workerManager()
                ->workerSettings()
                ->setDataDirectoryPath(workerDataDirPath);

            WorkerAgent::create(LWEDelegate::g_starfishInstance);
        }
        return 0;
    });
#else
    if (Starfish::WorkerAgent::isCreated() == false) {
        LWEDelegate::g_starfishInstance->workerManager()
            ->workerSettings()
            ->setDataDirectoryPath(workerDataDirPath);

        // TODO: Pass `starfish` only here. Do this now since `nullptr` is
        // passed when SW runs on a single process.
        WorkerAgent::create(LWEDelegate::g_starfishInstance);
    }
#endif
}

static void registerOnStatusChangedHandler(
    const std::function<void(WorkerProcessState)> &cb)
{
    STARFISH_RELEASE_ASSERT(WorkerAgent::isCreated());
    auto onStateChangedCallback = [cb](WorkerAgentState state) {
        cb(ToWorkerState(state));
    };

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([&]() -> size_t {
        STARFISH_ASSERT(cb != nullptr);
        WorkerAgent::instance()->registerOnStatusChangedHandler(
            onStateChangedCallback);
        return 0;
    });
#else
    WorkerAgent::instance()->registerOnStatusChangedHandler(
        onStateChangedCallback);
#endif
}

static void finalizeWorkerProcess()
{
#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([]() -> size_t {
        if (WorkerAgent::isCreated() == true) {
            WorkerAgent::instance()->destroy();
        }
        return 0;
    });
#else
    if (WorkerAgent::isCreated() == true) {
        WorkerAgent::instance()->destroy();
    }
#endif

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

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([dataDirectoryPath]() -> size_t {
        LWEDelegate::g_starfishInstance->workerManager()
            ->workerSettings()
            ->setDataDirectoryPath(dataDirectoryPath);
        return 0;
    });
#else
    LWEDelegate::g_starfishInstance->workerManager()
        ->workerSettings()
        ->setDataDirectoryPath(dataDirectoryPath);
#endif
}

void WorkerClient::RegisterServiceWorkerProcessExecutor(
    const std::function<bool()> &fn)
{
    STARFISH_RELEASE_ASSERT(LWEDelegate::LWE::IsInitialized());

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([&]() -> size_t {
        LWEDelegate::g_starfishInstance->workerManager()
            ->workerSettings()
            ->setServiceWorkerProcessExecutor(fn);
        return 0;
    });
#else
    LWEDelegate::g_starfishInstance->workerManager()
        ->workerSettings()
        ->setServiceWorkerProcessExecutor(fn);
#endif
}

#endif // defined(STARFISH_WEBWORKER_HOST)

} // namespace LWE

#endif
