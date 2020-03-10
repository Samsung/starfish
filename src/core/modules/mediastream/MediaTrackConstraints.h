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

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {
class ExecutionContext;

struct DoubleRange {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, max, Max)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, min, Min)

    double m_max{ 0 };
    double m_min{ 0 };
    bool m_hasMax{ false };
    bool m_hasMin{ false };
};

struct ConstrainDoubleRange : DoubleRange {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, exact, Exact)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, ideal, Ideal)

    double m_exact{ 0 };
    double m_ideal{ 0 };
    bool m_hasExact{ false };
    bool m_hasIdeal{ false };
};

struct ULongRange {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(unsigned int, max, Max)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(unsigned int, min, Min)

    unsigned int m_max{ 0 };
    unsigned int m_min{ 0 };
    bool m_hasMax{ false };
    bool m_hasMin{ false };
};

struct ConstrainULongRange : ULongRange {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(unsigned int, exact, Exact)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(unsigned int, ideal, Ideal)

    unsigned int m_exact{ 0 };
    unsigned int m_ideal{ 0 };
    bool m_hasExact{ false };
    bool m_hasIdeal{ false };
};

struct ConstrainBooleanParameters {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, exact, Exact)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, ideal, Ideal)

    bool m_exact{ false };
    bool m_ideal{ false };
    bool m_hasExact{ false };
    bool m_hasIdeal{ false };
};

struct ConstrainDOMStringParameters {
};

struct MediaTrackConstraintSet {
};

struct MediaTrackConstraints : MediaTrackConstraintSet {
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<MediaTrackConstraintSet>,
                                      advanced, Advanced)

    GCVector<MediaTrackConstraintSet> m_advanced;
    bool m_hasAdvanced{ false };
};
} // namespace Starfish

#endif
#endif
