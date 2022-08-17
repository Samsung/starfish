/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "core/modules/serviceworker/cache/CachePolyfillLoader.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/serviceworker/util/Trace.h"

#include "binding/generated/Js2c_CacheStorage.h"

namespace Starfish {

bool CachePolyfillLoader::load(WorkerScriptController* controller)
{
    TRACE(SVCWORKER, "Load: s_js2c_cache_min_js");
    String* text = String::fromUTF8(s_js2c_cache_min_js.c_str(),
                                    s_js2c_cache_min_js.size());
    if (!controller->evaluatefromString(text)) {
        STARFISH_LOG_ERROR("Fail to load global script: js2c_cache_min_js");
        return false;
    }
    return true;
}

} // namespace Starfish

#endif
