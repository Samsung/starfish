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
