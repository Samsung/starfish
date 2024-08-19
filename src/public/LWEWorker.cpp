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

#if defined(STARFISH_WEBWORKER_HOST)

#include <cassert>

#include "LWEWorker.h"

#ifdef STARFISH_API_ENABLE_LOADER
#include "LWEWorkerDelegateLoader.h"
#else
#include "public/delegate/LWEWorkerDelegate.h"
#endif

#if defined(NDEBUG)
#define LWE_WORKER_ASSERT(assertion) ((void)0)
#else
#define LWE_WORKER_ASSERT(assertion) assert(assertion);
#endif

namespace LWE {

static void initializeWorkerProcess(const std::string &storageDirectoryPath)
{
#ifdef STARFISH_API_ENABLE_LOADER
#if defined(STARFISH_ENABLE_SHARED_WORKER)
    std::string targetName = STARFISH_SHARED_WORKER_API_TARGET_NAME;
#elif defined(STARFISH_ENABLE_SERVICE_WORKER)
    std::string targetName = STARFISH_SERVICE_WORKER_API_TARGET_NAME;
#else
#error \
    "Please define STARFISH_ENABLE_SHARED_WORKER or STARFISH_ENABLE_SERVICE_WORKER."
#endif

    if (!LWEWorkerDelegateLoader::getInstance()->load(targetName)) {
        LWE_WORKER_ASSERT(false);
    }

    LWEWorkerDelegateLoader::getSafeInstance()->kLWEWorkerProcTable.Initialize(
        storageDirectoryPath);

#else
    LWEDelegate::LWEWorker::Initialize(storageDirectoryPath);
#endif
}

static void registerOnStatusChangedHandler(
    const std::function<void(WorkerProcessState)> &cb)
{
#ifdef STARFISH_API_ENABLE_LOADER
    return LWEWorkerDelegateLoader::getSafeInstance()
        ->kLWEWorkerProcTable.RegisterOnStatusChangedHandler(cb);
#else
    return LWEDelegate::LWEWorker::RegisterOnStatusChangedHandler(cb);
#endif
}

static void finalizeWorkerProcess()
{
#ifdef STARFISH_API_ENABLE_LOADER
    return LWEWorkerDelegateLoader::getSafeInstance()
        ->kLWEWorkerProcTable.Finalize();
#else
    return LWEDelegate::LWEWorker::Finalize();
#endif
}

#if defined(STARFISH_ENABLE_SHARED_WORKER)

void SharedWorker::Initialize(const std::string &storageDirectoryPath)
{
    return initializeWorkerProcess(storageDirectoryPath);
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

#elif defined(STARFISH_ENABLE_SERVICE_WORKER)

void ServiceWorker::Initialize(const std::string &storageDirectoryPath)
{
    return initializeWorkerProcess(storageDirectoryPath);
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

#endif

} // namespace LWE

#endif
