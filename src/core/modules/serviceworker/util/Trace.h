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

#pragma once

#include "core/modules/serviceworker/util/Logger.h"

class Trace : public Logger {
public:
    Trace(std::string id);
    Trace(std::string id, const char* functionName, const char* filename,
          const int line);
};

namespace Starfish {

#if defined(NDEBUG)

#undef LOGI
#define LOGI(id, ...)
#define TRACE(id, ...)
#define TRACE_SCOPE(id, ...)

#else

#define LOGI(id, ...) Trace(#id).print(__VA_ARGS__)

#define TRACE(id, ...) \
    Trace(#id, __PRETTY_FUNCTION__, __FILE_NAME__, __LINE__).print(__VA_ARGS__)

#define TRACE_SCOPE(id, ...)      \
    IndentCounter __counter(#id); \
    TRACE(id, __VA_ARGS__)

#endif

} // namespace Starfish
