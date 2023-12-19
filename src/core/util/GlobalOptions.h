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

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <sstream>
#include <map>
#include <set>

namespace Starfish {

struct EXPORT ValueGroup {
    std::set<std::string> positives;
    std::set<std::string> negatives;
    bool includeAsteriskInPositives{ false };
    std::string raw;
};

class EXPORT GlobalOptions {
public:
    static GlobalOptions& instance();
    bool has(const char* key, const char* subKey = nullptr,
             bool isAsteriskSupported = true);
    std::string get(const char* key);

    template <typename T>
    void set(const char* key, const T& value)
    {
        assert(key != nullptr);
        std::ostringstream out;
        out << value;
        parse(key, out.str().c_str());
    }

private:
    GlobalOptions();
    void readEnvironmentValue(const char* key);
    void parse(const char* key, const char* value);
    std::map<std::string, std::shared_ptr<ValueGroup>> m_valueGroup;
};

} // namespace Starfish
