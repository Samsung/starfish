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

GlobalOptions& GlobalOptions::instance()
{
    static GlobalOptions instance;
    return instance;
}

GlobalOptions::GlobalOptions()
{
#if !defined(NDEBUG)
    const char* verbose;

    verbose = getenv("DEBUG_WORKER");
    if ((verbose != nullptr) && (strlen(verbose) > 0)) {
        set("DEBUG_WORKER", std::atoi(verbose));
    } else {
        set("DEBUG_WORKER", 0);
    }

    verbose = getenv("DEBUG_CAST");
    if ((verbose != nullptr) && (strlen(verbose) > 0)) {
        set("DEBUG_CAST", std::atoi(verbose));
    } else {
        set("DEBUG_CAST", 0);
    }

    verbose = getenv("DEBUG_CAST_TARGET_IP");
    if ((verbose != nullptr) && (strlen(verbose) > 0)) {
        set("DEBUG_CAST_TARGET_IP", verbose);
    } else {
        set("DEBUG_CAST_TARGET_IP", 0);
    }
#endif
}

} // namespace Starfish
