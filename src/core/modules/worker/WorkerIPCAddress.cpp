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

#if defined(STARFISH_USE_WORKER_PROCESS)

#include "StarfishConfig.h"
#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/util/debug/Trace.h"
#include "core/modules/worker/util/LocalStorageHelper.h"
#include "core/modules/worker/WorkerIPCAddress.h"

namespace Starfish {

WorkerIPCAddress::WorkerIPCAddress(WorkerSettings* settings,
                                   const std::string& resourceDirPath)
    : m_workerSettings(settings)
    , m_resourceDirPath(resourceDirPath)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            WorkerIPCAddress* self = castTo<WorkerIPCAddress*>(obj);
            self->~WorkerIPCAddress();
        },
        NULL, NULL, NULL);
}

WorkerIPCAddress::~WorkerIPCAddress() = default;

const std::string WorkerIPCAddress::getIPCHandlePath(const std::string& last)
{
    std::stringstream ss;

    ss << m_workerSettings->dataDirectoryPath() << m_resourceDirPath << "/"
       << last;

    return ss.str();
}

const std::string WorkerIPCAddress::createIPCAddress(const std::string& last)
{
    std::stringstream ss;

    ss << "ipc://" << getIPCHandlePath(last);

    TRACE(IPC, ss.str());
    return ss.str();
}

void WorkerIPCAddress::acquire()
{
    // TODO: consider making parent directories as needed.
    LocalStorageHelper::File::mkdirIfNotExists(
        m_workerSettings->dataDirectoryPath());

    LocalStorageHelper::File::createClearDirectory(getIPCHandlePath());

    m_workerSettings->addOnChangeDataDirectoryPathCallback(
        [this](const std::string& curPath, const std::string& newPath) {
            LocalStorageHelper::File::remove(curPath);
            LocalStorageHelper::File::createClearDirectory(newPath);

            LocalStorageHelper::File::mkdirIfNotExists(getIPCHandlePath());
            TRACE(IPC, "Create", getIPCHandlePath());
        });
}

void WorkerIPCAddress::release()
{
    // release the directory for ipc handles
    LocalStorageHelper::File::remove(getIPCHandlePath());
    TRACE(IPC, "Remove", getIPCHandlePath());
}

} // namespace Starfish

#endif
