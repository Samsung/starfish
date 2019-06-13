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
#include "core/modules/cast/DIALRunnable.h"

namespace Starfish {

#define CAST_DIAL_BUFFER_SIZE 256

DIALRunnable::DIALRunnable(MessageLoop* messageLoop, CastConfig* config)
    : BaseRunnable(messageLoop)
    , m_server(new httplib::Server())
    , m_config(config)
{
    STARFISH_ASSERT(messageLoop != nullptr);
    STARFISH_ASSERT(m_server != nullptr);
    STARFISH_ASSERT(config != nullptr);
}

bool DIALRunnable::doRun()
{
    char buffer[CAST_DIAL_BUFFER_SIZE];
    snprintf(buffer, CAST_DIAL_BUFFER_SIZE, "http://%s:%d%s",
             m_config->localAddress()->toUTF8NonGCString().data(),
             LOCATION_PORT, CAST_APP_URL);
    std::string appURL(buffer);

    m_server->Get(LOCATION_DESC, [appURL](const httplib::Request& req,
                                          httplib::Response& res) {
        STARFISH_LOG_INFO("CAST - GET: %s\n", LOCATION_DESC);

        res.set_header("Application-URL", appURL.data());
        res.set_content(CastConfig::templateDeviceDescription,
                        strlen(CastConfig::templateDeviceDescription),
                        "text/xml");
    });

    CastApplication youtubeApp(m_server, std::string("YouTube"));

#if !defined(NDEBUG)
    m_server->Get(R"(/(.*))",
                  [](const httplib::Request& req, httplib::Response& res) {
                      STARFISH_LOG_INFO("CAST - GET: %s\n", req.path.data());
                  });

    m_server->Post(R"(/(.*))",
                   [](const httplib::Request& req, httplib::Response& res) {
                       STARFISH_LOG_INFO("CAST - POST: %s\n", req.path.data());
                   });
#endif

    m_server->listen(m_config->localAddress()->toUTF8NonGCString().data(),
                     LOCATION_PORT);
    return false;
}

void DIALRunnable::stop()
{
    BaseRunnable::stop();

    m_server->stop();
    delete m_server;
}

} // namespace Starfish

#endif
