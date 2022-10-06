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

ServiceWorkerOption::ServiceWorkerOption()
{
    initLocalStorageRootDir();
}

void ServiceWorkerOption::initLocalStorageRootDir()
{
    if (GlobalOptions::instance().has("SW_STORAGE")) {
        m_localStorageRootDir = GlobalOptions::instance().get("SW_STORAGE");
    } else {
        const char* homeDirPath = getenv("HOME");
        if (!homeDirPath || strlen(homeDirPath) == 0) {
            m_localStorageRootDir = "/tmp";
        } else {
            m_localStorageRootDir = homeDirPath;
        }
        m_localStorageRootDir += "/Starfish-sw-cache";
    }
    TRACE(SVCWORKER, m_localStorageRootDir.data());
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
