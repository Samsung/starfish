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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishProgramOptions__)
#define __StarfishProgramOptions__

#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Starfish {

class ProgramOptions : public gc {
public:
    typedef int (*logger_t)(const char*, ...);
    typedef std::unordered_map<std::string, std::string> map_t;

    ProgramOptions();

    bool has(const std::string& key);

    template <typename T = std::string>
    T get(const std::string& key)
    {
        T converted;
        std::istringstream in(m_map[key]);
        in >> converted >> std::ws;
        return converted;
    }

    template <typename T>
    void set(const std::string& key, const T& value)
    {
        m_map[key] = value;
    }

private:
    map_t m_map;
    logger_t m_logger;
};

} // namespace Starfish

#endif
