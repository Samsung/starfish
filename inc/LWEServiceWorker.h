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

#ifndef __LWE_SERVICE_WORKER__
#define __LWE_SERVICE_WORKER__

#include "LWEWebView.h"

namespace LWE {

class LWE_EXPORT ServiceWorkerClient {
public:
    /*
     * Register service worker data directory path.
     *
     * Be sure to set the same data path as the service worker server.
     * If you do not register data directory, the data directory path is
     * set to '${HOME}/starfish-sw-data' or /tmp/starfish-sw-data.
     *
     * This method must be invoked after LWE::Initialize() is invoked.
     */
    static void RegisterDataDirectoryPath(const std::string &dataDirectoryPath);

    /*
     * Register service worker server process executor callback function.
     * The callback function should return the success or failure of the
     * processor execution.
     *
     * This method must be invoked after LWE::Initialize() is invoked.
     */
    static void RegisterServiceWorkerProcessExecutor(
        const std::function<bool()> &fn);
};

class LWE_EXPORT ServiceWorker {
public:
    enum class State {
        None,
        Terminated,
    };

    /*
     * Initialize service worker server.
     *
     * Be sure to set the same data path as the service worker client.
     * If you set data directory path to an empty path, it is set to
     * '${HOME}/starfish-sw-data' or /tmp/starfish-sw-data.
     */
    static void Initialize(const std::string &dataDirectoryPath);

    static void RegisterOnStatusChangedHandler(
        const std::function<void(State)> &cb);

    static void Finalize();
};

} // namespace LWE

#endif
