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

#include "StarfishConfig.h"

#include "core/util/ProgramOptions.h"

LoggerOption* LoggerOption::instance()
{
    static LoggerOption s_instance;
    return &s_instance;
}

bool LoggerOption::isLogEnable(const char* location)
{
    if (m_useFilter == false || m_filters.find(location) != m_filters.end()) {
        return true;
    }
    return false;
}

static std::vector<std::string> strSplit(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void LoggerOption::registerFilterString(const std::string rawKeys)
{
    for (const auto& token : strSplit(rawKeys, ',')) {
        m_filters.emplace(token);
    }
    m_useFilter = (m_filters.size() > 0);
}

void LoggerOption::parseEnv()
{
    const char* filterKeys = getenv("STARFISH_LOG");
    if (filterKeys != nullptr) {
        STARFISH_LOG_WARN("LOGGER FILTER: ON (%s)", filterKeys);
        registerFilterString(filterKeys);
    }
}

namespace Starfish {

bool ProgramOptions::has(const char* key)
{
    STARFISH_ASSERT(key != nullptr);
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        return true;
    }
    return false;
}

bool ProgramOptions::is(const char* key)
{
    STARFISH_ASSERT(key != nullptr);
    return get<bool>(key);
}

} // namespace Starfish

#endif
