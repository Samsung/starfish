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

#ifndef __StarfishMediaTrackDoubleRange__
#define __StarfishMediaTrackDoubleRange__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class ExecutionContext;

struct DoubleRange : public gc {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, max, Max)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, min, Min)

    double m_max{ 0 };
    double m_min{ 0 };
    bool m_hasMax{ false };
    bool m_hasMin{ false };
};
} // namespace Starfish

#endif
#endif
