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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "core/util/GlobalOptions.h"

namespace Starfish {

#define GETTIME()                                            \
    std::chrono::duration<double>(                           \
        std::chrono::system_clock::now().time_since_epoch()) \
        .count()

#define CLR_RESET "\033[0m"
#define CLR_LIGHT_YELLOW "\033[1;33m"

#if !defined(NDEBUG)

#define FMTTIME(now, down) (now - (((int)(now / down)) * down))

#define LOG_IF_ALLOWED(condition, name, fmt, ...)                             \
    do {                                                                      \
        if (condition) {                                                      \
            auto now = GETTIME();                                             \
            STARFISH_LOG_INFO(CLR_LIGHT_YELLOW "%.3lf %s %s: " fmt CLR_RESET, \
                              FMTTIME(now, 100), name, __FUNCTION__,          \
                              ##__VA_ARGS__);                                 \
        }                                                                     \
    } while (0)

#define WORKER_LOG_IF_ALLOWED(lvl, fmt, ...)                                  \
    LOG_IF_ALLOWED(GlobalOptions::instance().get<int>("DEBUG_WORKER") >= lvl, \
                   GlobalOptions::instance().get("app").c_str(), fmt,         \
                   ##__VA_ARGS__)

#define SWCLIENT_LOG_IF_ALLOWED(lvl, fmt, ...)                                \
    LOG_IF_ALLOWED(GlobalOptions::instance().get<int>("DEBUG_WORKER") >= lvl, \
                   "CLIT", fmt, ##__VA_ARGS__)

#define SWHOST_LOG_IF_ALLOWED(lvl, fmt, ...)                                  \
    LOG_IF_ALLOWED(GlobalOptions::instance().get<int>("DEBUG_WORKER") >= lvl, \
                   "HOST", fmt, ##__VA_ARGS__)

#else // else defined(NDEBUG)

#define LOG_IF_ALLOWED(condition, fmt, ...)
#define WORKER_LOG_IF_ALLOWED(fmt, ...)
#define SWCLIENT_LOG_IF_ALLOWED(fmt, ...)
#define SWHOST_LOG_IF_ALLOWED(fmt, ...)

#endif

} // namespace Starfish

#endif
