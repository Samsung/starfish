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

#if defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "LWEWorker.h"
#include "LWEDelegate.h"
#include "LWEWorkerDelegate.h"

#include "ThreadedCallHelper.h"

#include "core/modules/worker/host/WorkerAgent.h"

#if defined(STARFISH_WINDOWS)
#include <fontconfig/fontconfig.h>
#endif

// using namespace Starfish;

namespace LWEDelegate {
extern Starfish::Starfish* g_starfishInstance;
}

namespace LWEDelegate {

static ::LWE::WorkerProcessState ToWorkerState(Starfish::WorkerAgentState state)
{
    switch (state) {
    case Starfish::WorkerAgentState::Terminated:
        return ::LWE::WorkerProcessState::Terminated;
    default:
        return ::LWE::WorkerProcessState::None;
    }
}

void LWEWorker::Initialize(const std::string& storageDirectoryPath)
{
    STARFISH_RELEASE_ASSERT(!LWEDelegate::LWE::IsInitialized());

    STARFISH_LOG_INFO("WORKER STARTS");

    LWEDelegate::LWE::Initialize(storageDirectoryPath.c_str());

    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        []() -> void {
            if (Starfish::WorkerAgent::isCreated() == false) {
                Starfish::WorkerAgent* agent = Starfish::WorkerAgent::create(
                    LWEDelegate::g_starfishInstance);
                agent->start();
            }
        });
}

void LWEWorker::RegisterOnStatusChangedHandler(
    const std::function<void(::LWE::WorkerProcessState)>& cb)
{
    STARFISH_RELEASE_ASSERT(Starfish::WorkerAgent::isCreated());

    auto onStateChangedCallback = [cb](Starfish::WorkerAgentState state) {
        cb(ToWorkerState(state));
    };

    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        [&]() -> void {
            Starfish::WorkerAgent::instance()->registerOnStatusChangedHandler(
                onStateChangedCallback);
        });
}

void LWEWorker::Finalize()
{
    STARFISH_RELEASE_ASSERT(Starfish::WorkerAgent::isCreated());

    LWEDelegate::ThreadedCallHelper::Instance()->PostTaskToLWEMainThreadSync(
        []() -> void {
            if (Starfish::WorkerAgent::isCreated() == true) {
                Starfish::WorkerAgent::instance()->destroy();
            }
        });

    LWEDelegate::LWE::Finalize();

    STARFISH_LOG_INFO("WORKER ENDS");
}

} // namespace LWEDelegate

extern "C" {
void LWEWorkerDelegate_LWEWorker_Initialize(
    const std::string& storageDirectoryPath)

{
    LWEDelegate::LWEWorker::Initialize(storageDirectoryPath);
}

void LWEWorkerDelegate_LWEWorker_RegisterOnStatusChangedHandler(
    const std::function<void(::LWE::WorkerProcessState)>& cb)
{
    LWEDelegate::LWEWorker::RegisterOnStatusChangedHandler(cb);
}

void LWEWorkerDelegate_LWEWorker_Finalize()
{
    LWEDelegate::LWEWorker::Finalize();
}
}

#endif
