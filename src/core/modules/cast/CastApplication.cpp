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

#ifdef STARFISH_ENABLE_CAST_SERVICE

#include <httplib.h>
#include "StarfishConfig.h"

#include "core/modules/cast/CastConfig.h"
#include "core/modules/cast/CastApplication.h"

namespace Starfish {

#define CAST_APP_INFOR_BUFFER_SIZE 1024

CastApplication::CastApplication(httplib::Server* server,
                                 const std::string& appName)
{
    STARFISH_ASSERT(server != nullptr);

    char appPathBuffer[CAST_APP_INFOR_BUFFER_SIZE];
    snprintf(appPathBuffer, CAST_APP_INFOR_BUFFER_SIZE, "/apps/%s",
             appName.data());
    server->Get(appPathBuffer, [appName](const httplib::Request& req,
                                         httplib::Response& res) {
        STARFISH_LOG_INFO("CAST - GET:/apps/%s\n", appName.data());

        char contentBuffer[CAST_APP_INFOR_BUFFER_SIZE];
        snprintf(contentBuffer, CAST_APP_INFOR_BUFFER_SIZE,
                 CastConfig::templateCastAppInfo, appName.data(), "stopped");

        res.set_content(contentBuffer, strlen(contentBuffer), "test/xml");
    });
}

} // namespace Starfish

#endif
