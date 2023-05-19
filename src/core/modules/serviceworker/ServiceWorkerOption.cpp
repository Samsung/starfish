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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/threading/Thread.h"
#include "core/util/GlobalOptions.h"
#include "core/modules/serviceworker/ServiceWorkerOption.h"

namespace Starfish {

std::string ServiceWorkerOption::getDefaultDataDirectoryPath()
{
    std::string dataDirectoryPath;

    const char *homeDirectoryPath = getenv("HOME");
    if (!homeDirectoryPath || strlen(homeDirectoryPath) == 0) {
        dataDirectoryPath = "/tmp";
    } else {
        dataDirectoryPath = homeDirectoryPath;
    }
    dataDirectoryPath += "/starfish-sw-data";

    TRACE(SVCWORKER, dataDirectoryPath.data());

    return dataDirectoryPath;
}

ServiceWorkerOption::ServiceWorkerOption(const std::string &dataDirectoryPath)
    : m_dataDirectoryPath(dataDirectoryPath)
{
    if (m_dataDirectoryPath.empty()) {
        m_dataDirectoryPath = getDefaultDataDirectoryPath();
    }
}

void ServiceWorkerOption::setDataDirectoryPath(const std::string &path)
{
    if (m_dataDirectoryPath == path) {
        return;
    }

    TRACEF(SVCWORKER, "Change service worker working dir: %s -> %s",
           m_dataDirectoryPath.data(), path.data());
    m_dataDirectoryPath = path;

    for (auto cb : m_onChangeDataDirectoryPathCallbacks) {
        cb(m_dataDirectoryPath);
    }
}

void ServiceWorkerOption::addOnChangeDataDirectoryPathCallback(
    OnChangeDataDirectoryPathCallback callback)
{
    m_onChangeDataDirectoryPathCallbacks.push_back(callback);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
