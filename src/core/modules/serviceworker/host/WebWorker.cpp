/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifdef STARFISH_ENABLE_WEBWORKER_HOST

#include "StarfishBase.h"
#include "core/modules/serviceworker/host/WebWorker.h"
#include <uv.h>

namespace Starfish {

WebWorker::WebWorker()
{
}

WebWorker::~WebWorker()
{
}

bool WebWorker::init()
{
    STARFISH_LOG_INFO("WebWorker::init\n");
    return true;
}

bool WebWorker::start()
{
    STARFISH_LOG_INFO("WebWorker::start\n");
    return true;
}

bool WebWorker::tick()
{
    int more = uv_run(uv_default_loop(), UV_RUN_ONCE);
    if (more == 0) {
        more = uv_loop_alive(uv_default_loop());
    }
    return more;
}

bool WebWorker::terminate()
{
    STARFISH_LOG_INFO("WebWorker::terminate\n");
    uv_stop(uv_default_loop());
    return true;
}

bool WebWorker::run()
{
    int more = 0;

    init();
    start();

    do {
        more = tick();
    } while (more);

    return terminate();
}

bool WebWorker::pause()
{
    STARFISH_LOG_INFO("WebWorker::pause\n");
    return true;
}

bool WebWorker::resume()
{
    STARFISH_LOG_INFO("WebWorker::resume\n");
    return true;
}

} // namespace Starfish

#endif
