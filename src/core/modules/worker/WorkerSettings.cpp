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

#ifdef STARFISH_USE_WORKER_PROCESS

#include "StarfishConfig.h"
#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/WorkerSettings.h"

namespace Starfish {

std::string WorkerSettings::getDefaultDataDirectoryPath()
{
    std::string dataDirectoryPath;

    const char *homeDirectoryPath = getenv("HOME");
    if (!homeDirectoryPath || strlen(homeDirectoryPath) == 0) {
        dataDirectoryPath = "/tmp";
    } else {
        dataDirectoryPath = homeDirectoryPath;
    }
    dataDirectoryPath += "/starfish-worker-data";

    TRACE(SVCWORKER, dataDirectoryPath.data());

    return dataDirectoryPath;
}

WorkerSettings::WorkerSettings()
    : m_dataDirectoryPath(getDefaultDataDirectoryPath())
{
}

WorkerSettings::WorkerSettings(const std::string &dataDirectoryPath)
    : m_dataDirectoryPath(dataDirectoryPath)
{
    if (m_dataDirectoryPath.empty()) {
        m_dataDirectoryPath = getDefaultDataDirectoryPath();
    }
}

void WorkerSettings::setDataDirectoryPath(const std::string &path)
{
    if (m_dataDirectoryPath == path) {
        return;
    }

    TRACEF(SVCWORKER, "Change worker working dir: %s -> %s",
           m_dataDirectoryPath.data(), path.data());

    const std::string curPath = m_dataDirectoryPath;
    m_dataDirectoryPath = path;

    for (auto cb : m_onChangeDataDirectoryPathCallbacks) {
        cb(curPath, m_dataDirectoryPath);
    }
}

void WorkerSettings::addOnChangeDataDirectoryPathCallback(
    OnChangeDataDirectoryPathCallback callback)
{
    m_onChangeDataDirectoryPathCallbacks.push_back(callback);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_WORKER
