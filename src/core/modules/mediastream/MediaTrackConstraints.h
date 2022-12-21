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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishMediaTrackConstraints__
#define __StarfishMediaTrackConstraints__

#include "binding/ScriptWrappable.h"
#include "core/modules/mediastream/MediaTrackConstraintSet.h"

namespace Starfish {
class ExecutionContext;

struct MediaTrackConstraints : MediaTrackConstraintSet {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<MediaTrackConstraintSet>,
                                      advanced, Advanced)

    GCVector<MediaTrackConstraintSet> m_advanced;
    bool m_hasAdvanced{ false };
};
} // namespace Starfish

#endif
#endif
