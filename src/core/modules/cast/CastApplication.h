/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CAST_SERVICE) && \
    !defined(__StarfishCastApplication__)
#define __StarfishCastApplication__

#include "platform/process/base/ProcessType.h"

namespace httplib {
class Server;
}

namespace Starfish {

class CastConfig;

struct CastAppInfo {
    CastAppInfo(const char* appName, const char* launchURL,
                const std::string& localAddress)
        : m_pid(-1)
        , m_appName(appName)
        , m_launchURL(launchURL)
        , m_localAddress(localAddress)
        , m_isRunning(false)
    {
    }
    PID m_pid;
    std::string m_appName;
    std::string m_launchURL;
    std::string m_localAddress;
    bool m_isRunning;
};

class CastApplication {
public:
    CastApplication(httplib::Server* server, CastConfig* config,
                    const char* appName, const char* launchURL);

private:
    CastAppInfo m_castAppInfo;
};

} // namespace Starfish
#endif
