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

static std::string dumpHTTPHeaders(const httplib::Headers& headers)
{
    std::string str;
    char buffer[BUFSIZ];

    for (const auto& pair : headers) {
        snprintf(buffer, sizeof(buffer), "%s: %s\n", pair.first.c_str(),
                 pair.second.c_str());
        str += buffer;
    }

    return str;
}

static std::string logRequestAndResponse(const httplib::Request& req,
                                         const httplib::Response& res)
{
    std::string str;
    char buffer[BUFSIZ];

    // Request
    str += COLOR_YELLOW "================================\n";

    snprintf(buffer, sizeof(buffer), "%s %s %s", req.method.c_str(),
             req.version.c_str(), req.path.c_str());
    str += buffer;

    std::string query;

    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        snprintf(buffer, sizeof(buffer), "%c%s=%s",
                 (it == req.params.begin()) ? '?' : '&', it->first.c_str(),
                 it->second.c_str());
        query += buffer;
    }

    snprintf(buffer, sizeof(buffer), "%s\n", query.c_str());
    str += buffer;
    str += dumpHTTPHeaders(req.headers);

    // Response
    str += COLOR_GREEN "--------------------------------\n";

    snprintf(buffer, sizeof(buffer), "%d %s\n", res.status,
             res.version.c_str());
    str += buffer;
    str += dumpHTTPHeaders(res.headers);
    str += "\n";

    if (!res.body.empty()) {
        str += res.body;
    }
    str += "\n";

    return str;
}

CastApplication::CastApplication(httplib::Server* server, CastConfig* config,
                                 const std::string& appName,
                                 const std::string& launch)
    : m_config(config)
    , m_appName(appName)
    , m_launch(launch)
    , m_isRunning(false)
{
    char appPathBuffer[CAST_APP_INFOR_BUFFER_SIZE];
    snprintf(appPathBuffer, CAST_APP_INFOR_BUFFER_SIZE, "/apps/%s",
             appName.data());

    server->Get(appPathBuffer, [this](const httplib::Request& req,
                                      httplib::Response& res) {
        CAST_LOG_IF_ALLOWED(3, "CAST - GET:/apps/%s\n", m_appName.data());

        const char* status = m_isRunning ? "running" : "stopped";
        char contentBuffer[CAST_APP_INFOR_BUFFER_SIZE];
        snprintf(contentBuffer, CAST_APP_INFOR_BUFFER_SIZE,
                 CastConfig::templateCastAppInfo, m_appName.data(), status);

        CAST_SEND_LOG_IF_ALLOWED(4, "%s\n", contentBuffer);

        res.set_content(contentBuffer, strlen(contentBuffer), "test/xml");
    });

    server->Post(appPathBuffer, [this](const httplib::Request& req,
                                       httplib::Response& res) {
        CAST_LOG_IF_ALLOWED(3, "CAST - POST:/apps/%s\n", m_appName.data());

        m_isRunning = true;
        char buffer[CAST_APP_INFOR_BUFFER_SIZE];
        snprintf(buffer, CAST_APP_INFOR_BUFFER_SIZE, "http://%s:%d/apps/%s/run",
                 m_config->localAddress()->toUTF8NonGCString().data(),
                 LOCATION_PORT, m_appName.data());
        res.status = 201;
        res.set_header("LOCATION", buffer);
        res.set_content(buffer, "text/html");

        if (req.has_param("v") && req.has_param("pairingCode")) {
            // TODO: Launch app
            CAST_LOG_IF_ALLOWED(3, "v: %s\n", req.get_param_value("v").data());
        }
    });

    char appRunPathBuffer[CAST_APP_INFOR_BUFFER_SIZE];
    snprintf(appRunPathBuffer, CAST_APP_INFOR_BUFFER_SIZE, "/apps/%s/run",
             appName.data());

    server->Delete(appRunPathBuffer,
                   [this](const httplib::Request& req, httplib::Response& res) {
                       CAST_LOG_IF_ALLOWED(3, "CAST - Delete\n");
                       m_isRunning = false;
                       res.status = 200;
                   });

#if !defined(NDEBUG)
    server->set_logger([](const httplib::Request& req,
                          const httplib::Response& res) {
        CAST_LOG_IF_ALLOWED(3, "%s\n", logRequestAndResponse(req, res).c_str());
    });
#endif
}

} // namespace Starfish

#endif
