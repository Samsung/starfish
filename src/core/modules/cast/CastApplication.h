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

#if defined(STARFISH_ENABLE_CAST_SERVICE) && \
    !defined(__StarfishCastApplication__)
#define __StarfishCastApplication__

namespace httplib {
class Server;
}

namespace Starfish {

class CastConfig;

class CastApplication {
public:
    CastApplication(httplib::Server* server, CastConfig* config,
                    const std::string& appName, const std::string& launch);

private:
    CastConfig* m_config;
    std::string m_appName;
    std::string m_launch;
    std::atomic_bool m_isRunning;
};

} // namespace Starfish
#endif
