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
#include "core/modules/location/Coordinates.h"
#include "core/dom/Document.h"

namespace Starfish {

Coordinates::Coordinates(Document* document, double latitude, double longitude,
                         Nullable<double> altitude, double accuracy,
                         Nullable<double> altitudeAccuracy,
                         Nullable<double> heading, Nullable<double> speed)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_latitude(latitude)
    , m_longitude(longitude)
    , m_altitude(altitude)
    , m_accuracy(accuracy)
    , m_altitudeAccuracy(altitudeAccuracy)
    , m_heading(heading)
    , m_speed(speed)
{
}

ScriptBindingInstance* Coordinates::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}
}
