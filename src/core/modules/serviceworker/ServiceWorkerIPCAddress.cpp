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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/util/debug/Trace.h"
#include "core/modules/worker/util/LocalStorageHelper.h"
#include "core/modules/serviceworker/ServiceWorkerIPCAddress.h"

namespace Starfish {

ServiceWorkerIPCAddress::ServiceWorkerIPCAddress(WorkerSettings* settings)
    : WorkerIPCAddress(settings, PATH_SERVICE_WORKER_IPC_DIR)
{
}

const std::string ServiceWorkerIPCAddress::getIPCHandlePath(
    const std::string& last)
{
    std::stringstream ss;

    ss << m_workerSettings->dataDirectoryPath() << m_resourceDirPath << "/";

    // Appends more parts.
#ifdef SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
    ss << "host";
#else
    ss << last;
#endif
    return ss.str();
}

const std::string ServiceWorkerIPCAddress::createIPCAddress(
    const std::string& last)
{
    std::stringstream ss;

#ifdef STARFISH_USE_WORKER_PROCESS
    // For Inter-Process Communication
    ss << "ipc://" << getIPCHandlePath(last);
#else
    // For In-Process Communication
    ss << "inproc://sw/";
#endif

    TRACE(IPC, ss.str());
    return ss.str();
}

void ServiceWorkerIPCAddress::release()
{
    TRACE_SCOPE(IPC);

    if (!GlobalOptions::instance().has("--leave-ipc-handle")) {
        LocalStorageHelper::File::remove(getIPCHandlePath());
        TRACE(IPC, "Remove", getIPCHandlePath());
    }
}

} // namespace Starfish

#endif
