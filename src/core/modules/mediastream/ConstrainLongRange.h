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
#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishMediaTrackConstrainLongRange__
#define __StarfishMediaTrackConstrainLongRange__

#include "binding/ScriptWrappable.h"
#include "core/modules/mediastream/ULongRange.h"

namespace Starfish {
class ExecutionContext;

struct ConstrainULongRange : ULongRange {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(unsigned int, exact, Exact)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(unsigned int, ideal, Ideal)

    unsigned int m_exact{ 0 };
    unsigned int m_ideal{ 0 };
    bool m_hasExact{ false };
    bool m_hasIdeal{ false };
};

} // namespace Starfish

#endif
#endif
