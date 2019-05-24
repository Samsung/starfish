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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "core/modules/serviceworker/ProgramOptions.h"
#include "core/modules/serviceworker/WorkerConfig.h"

namespace Starfish {

WorkerConfig& WorkerConfig::instance()
{
    static WorkerConfig instance;
    return instance;
}

WorkerConfig::WorkerConfig()
{
#if !defined(NDEBUG)
    const char* verbose = getenv("DEBUG_WORKER");

    if ((verbose != nullptr) && (strlen(verbose) > 0)) {
        set("DEBUG_WORKER", std::atoi(verbose));
        STARFISH_LOG_INFO("DEBUG_WORKER: %d\n", std::atoi(verbose));
    } else {
        set("DEBUG_WORKER", 0);
        STARFISH_LOG_INFO("DEBUG_WORKER: %d\n", 0);
    }

#endif
}

} // namespace Starfish

#endif
