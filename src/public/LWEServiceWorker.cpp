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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "LWEServiceWorker.h"
#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/PerProcess.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/ServiceWorkerOption.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/util/Trace.h"

using namespace Starfish;

namespace LWE {

extern Starfish::Starfish *g_starfishInstance;

#if defined(STARFISH_WEBWORKER_HOST)

static ServiceWorker::State ToServiceWorkerState(ServiceWorkerAgentState state)
{
    switch (state) {
    case ServiceWorkerAgentState::Terminated:
        return ServiceWorker::State::Terminated;
    default:
        return ServiceWorker::State::None;
    }
}

void ServiceWorker::Initialize(const std::string &dataDirectoryPath)
{
    std::string swDataDirPath = dataDirectoryPath;
    if (swDataDirPath.empty()) {
        swDataDirPath =
            Starfish::ServiceWorkerOption::getDefaultDataDirectoryPath();
    }

    LWE::Initialize(
        (swDataDirPath + "/starfish-serviceworker-local-storage.txt").c_str(),
        (swDataDirPath + "/starfish-serviceworker-cookie.txt").c_str(),
        (swDataDirPath + "/starfish-serviceworker-http-cache.txt").c_str());

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([swDataDirPath]() -> size_t {
        if (ServiceWorkerAgent::isCreated() == false) {
            g_starfishInstance->serviceWorkerOption()->setDataDirectoryPath(
                swDataDirPath);

            ServiceWorkerAgent::create(g_starfishInstance,
                                       g_starfishInstance->perProcess());
        }
        return 0;
    });
#else
    if (Starfish::ServiceWorkerAgent::isCreated() == false) {
        g_starfishInstance->serviceWorkerOption()->setDataDirectoryPath(
            swDataDirPath);

        // TODO: Pass `starfish` only here. Do this now since `nullptr` is
        // passed when SW runs on a single process.
        ServiceWorkerAgent::create(g_starfishInstance,
                                   g_starfishInstance->perProcess());
    }
#endif
}

void ServiceWorker::RegisterOnStatusChangedHandler(
    const std::function<void(ServiceWorker::State)> &cb)
{
    STARFISH_RELEASE_ASSERT(ServiceWorkerAgent::isCreated());
    auto onStateChangedCallback = [cb](ServiceWorkerAgentState state) {
        cb(ToServiceWorkerState(state));
    };

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([&]() -> size_t {
        STARFISH_ASSERT(cb != nullptr);
        ServiceWorkerAgent::instance()->registerOnStatusChangedHandler(
            onStateChangedCallback);
        return 0;
    });
#else
    ServiceWorkerAgent::instance()->registerOnStatusChangedHandler(
        onStateChangedCallback);
#endif
}

void ServiceWorker::Finalize()
{
#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([]() -> size_t {
        if (ServiceWorkerAgent::isCreated() == true) {
            ServiceWorkerAgent::instance()->destroy();
        }
        return 0;
    });
#else
    if (ServiceWorkerAgent::isCreated() == true) {
        ServiceWorkerAgent::instance()->destroy();
    }
#endif

    LWE::LWE::Finalize();
}

#else // !defined(STARFISH_WEBWORKER_HOST)
void ServiceWorkerClient::RegisterDataDirectoryPath(
    const std::string &dataDirectoryPath)
{
    STARFISH_RELEASE_ASSERT(LWE::IsInitialized());

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([dataDirectoryPath]() -> size_t {
        g_starfishInstance->serviceWorkerOption()->setDataDirectoryPath(
            dataDirectoryPath);
        return 0;
    });
#else
    g_starfishInstance->serviceWorkerOption()->setDataDirectoryPath(
        dataDirectoryPath);
#endif
}

void ServiceWorkerClient::RegisterServiceWorkerProcessExecutor(
    const std::function<bool()> &fn)
{
    STARFISH_RELEASE_ASSERT(LWE::IsInitialized());

#ifdef PORT_NEEDS_THREADED_PUBLIC_API
    MessageLoop::runOnMainThreadSync([&]() -> size_t {
        g_starfishInstance->serviceWorkerOption()
            ->setServiceWorkerProcessExecutor(fn);
        return 0;
    });
#else
    g_starfishInstance->serviceWorkerOption()->setServiceWorkerProcessExecutor(
        fn);
#endif
}

#endif // defined(STARFISH_WEBWORKER_HOST)

} // namespace LWE

#endif
