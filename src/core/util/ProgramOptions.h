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

#ifndef __StarfishProgramOptions__
#define __StarfishProgramOptions__

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class LoggerOption {
public:
    static LoggerOption* instance();
    bool isLogEnable(const char* location);
    void registerFilterString(const std::string rawKeys);
    void parseEnv();

private:
    LoggerOption() = default;
    std::unordered_set<std::string> m_filters;
    bool m_useFilter{ false };
};

namespace Starfish {

class ProgramOptions {
public:
    using map_t = std::unordered_map<std::string, std::string>;

    ProgramOptions() = default;
    virtual ~ProgramOptions() = default;

    bool has(const char* key);
    bool is(const char* key);

    template <typename T = std::string>
    T get(const char* key)
    {
        assert(key != nullptr);
        T converted;
        std::istringstream in(m_map[key]);
        in >> converted >> std::ws;
        return converted;
    }

    template <typename T>
    void set(const char* key, const T& value)
    {
        assert(key != nullptr);
        std::ostringstream out;
        out << value;
        m_map[key] = out.str();
    }

private:
    map_t m_map;
};

} // namespace Starfish

#endif
