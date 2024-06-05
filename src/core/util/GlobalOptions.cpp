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

#include "StarfishConfig.h"
#include "core/util/GlobalOptions.h"

namespace Starfish {

static const int kMaxStringLength = 256;

GlobalOptions& GlobalOptions::instance()
{
    static GlobalOptions instance;
    return instance;
}

GlobalOptions::GlobalOptions()
{
#if defined(NDEBUG)
#if defined(ENABLE_TRACE)
    readEnvironmentValue("TRACE");
#endif
#else
    readEnvironmentValue("TRACE");
    readEnvironmentValue("CACHE_MODULE_PATH");
    readEnvironmentValue("DEBUG_CAST");
    readEnvironmentValue("DEBUG_CAST_TARGET_IP");
#endif

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
    readEnvironmentValue("SW_STORAGE");
#endif
}

void GlobalOptions::readEnvironmentValue(const char* key)
{
    parse(key, getenv(key));
}

void GlobalOptions::parse(const char* key, const char* value)
{
    if ((value != nullptr) && (strnlen(value, kMaxStringLength) > 0)) {
        // parse a comma separated value.
        std::string token;
        std::stringstream ss(value);
        std::shared_ptr<ValueGroup> tokens = std::make_shared<ValueGroup>();

        while (std::getline(ss, token, ',')) {
            if (token.find('-') == 0) {
                tokens->negatives.insert(token.substr(1));
                continue;
            }
            tokens->positives.insert(token);
            if (token == "*") {
                tokens->includeAsteriskInPositives = true;
            }
        }

        tokens->raw = value;
        m_valueGroup[key] = tokens;
        STARFISH_LOG_INFO("[ %s = %s ]", key, value);
    }
}

bool GlobalOptions::has(const char* key, const char* subKey,
                        bool isAsteriskSupported)
{
    STARFISH_ASSERT(key != nullptr);

    std::shared_ptr<ValueGroup> tokens = m_valueGroup[key];
    if (!tokens) {
        return false;
    } else if (!subKey) {
        return true;
    }

    if (!tokens->positives.empty()) {
        /*
            // usage: isAsteriskSupported
            e.g) `export KEY=*,-SUBKEY`
        */
        if (!isAsteriskSupported || !tokens->includeAsteriskInPositives) {
            if (tokens->positives.find(subKey) == tokens->positives.end()) {
                return false;
            }
        }
    }

    if (!tokens->negatives.empty()) {
        if (tokens->negatives.find(subKey) != tokens->negatives.end()) {
            return false;
        }
    }

    return true;
}

std::string GlobalOptions::get(const char* key)
{
    STARFISH_ASSERT(key != nullptr);

    std::shared_ptr<ValueGroup> tokens = m_valueGroup[key];
    if (!tokens) {
        return "";
    }
    return tokens->raw;
}

} // namespace Starfish
