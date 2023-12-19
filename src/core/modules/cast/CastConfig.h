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

#if defined(STARFISH_ENABLE_CAST_SERVICE) && !defined(__StarfishCastConfig__)
#define __StarfishCastConfig__

#include "StarfishConfig.h"
#include "core/util/GlobalOptions.h"

namespace Starfish {

#define SSDP_GROUP "239.255.255.250"
#define SSDP_PORT 1900
#define SSDP_ST "urn:dial-multiscreen-org:service:dial:1"

#define LOCATION_PORT 5696
#define LOCATION_DESC "/deviceDescription.xml"
#define CAST_APP_URL "/apps"

class CastConfig : public gc {
public:
    CastConfig() = default;

    static const char* templateDeviceDescription;
    static const char* templateMSearchResponse;
    static const char* templateCastAppInfo;

    DEFINE_GETTER_SETTER(String*, localAddress, LocalAddress);

private:
    String* m_localAddress{ String::emptyString };
};

#define LOG_ID "DEBUG_CAST"

#define GETTIME()                                            \
    std::chrono::duration<double>(                           \
        std::chrono::system_clock::now().time_since_epoch()) \
        .count()

#define COLOR_RESET "\033[0m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"

#if !defined(NDEBUG)

#define FMTTIME(now, down) (now - (((int)(now / down)) * down))

#define LOG_IF_ALLOWED(condition, fmt, ...)                    \
    do {                                                       \
        if (condition) {                                       \
            auto now = GETTIME();                              \
            STARFISH_LOG_INFO(fmt COLOR_RESET, ##__VA_ARGS__); \
        }                                                      \
    } while (0)

#define CAST_LOG_IF_ALLOWED(lvl, fmt, ...)                                   \
    LOG_IF_ALLOWED((GlobalOptions::instance().get<int>(LOG_ID) >= lvl), fmt, \
                   ##__VA_ARGS__)

#define CAST_SEND_LOG_IF_ALLOWED(lvl, fmt, ...) \
    CAST_LOG_IF_ALLOWED(lvl, COLOR_GREEN fmt, ##__VA_ARGS__)

#define CAST_RECV_LOG_IF_ALLOWED(lvl, fmt, ...) \
    CAST_LOG_IF_ALLOWED(lvl, COLOR_YELLOW fmt, ##__VA_ARGS__)

#else // else defined(NDEBUG)

#define LOG_IF_ALLOWED(condition, fmt, ...)
#define CAST_LOG_IF_ALLOWED(fmt, ...)
#define CAST_SEND_LOG_IF_ALLOWED(lvl, fmt, ...)
#define CAST_RECV_LOG_IF_ALLOWED(lvl, fmt, ...)

#endif

} // namespace Starfish

#endif
