/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCoordinates__
#define __StarfishCoordinates__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class Document;

class Coordinates : public ScriptWrappable {
public:
    Coordinates(Document* document, double latitude, double longitude,
                Optional<double> altitude, double accuracy,
                Optional<double> altitudeAccuracy, Optional<double> heading,
                Optional<double> speed);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Coordinates)

    double latitude()
    {
        return m_latitude;
    }

    double longitude()
    {
        return m_longitude;
    }

    Optional<double> altitude()
    {
        return m_altitude;
    }

    double accuracy()
    {
        return m_accuracy;
    }

    Optional<double> altitudeAccuracy()
    {
        return m_altitudeAccuracy;
    }

    Optional<double> heading()
    {
        if (m_speed.hasValue() && m_speed.getValue() == 0) {
            return Optional<double>(std::numeric_limits<double>::quiet_NaN());
        }
        return m_heading;
    }

    Optional<double> speed()
    {
        return m_speed;
    }

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    double m_latitude;
    double m_longitude;
    Optional<double> m_altitude;
    double m_accuracy;
    Optional<double> m_altitudeAccuracy;
    Optional<double> m_heading;
    Optional<double> m_speed;
};
} // namespace Starfish

#endif
