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
                Nullable<double> altitude, double accuracy,
                Nullable<double> altitudeAccuracy, Nullable<double> heading,
                Nullable<double> speed);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Coordinates)

    double latitude()
    {
        return m_latitude;
    }

    double longitude()
    {
        return m_longitude;
    }

    Nullable<double> altitude()
    {
        return m_altitude;
    }

    double accuracy()
    {
        return m_accuracy;
    }

    Nullable<double> altitudeAccuracy()
    {
        return m_altitudeAccuracy;
    }

    Nullable<double> heading()
    {
        if (m_speed.hasValue() && m_speed.getValue() == 0) {
            return Nullable<double>(std::numeric_limits<double>::quiet_NaN());
        }
        return m_heading;
    }

    Nullable<double> speed()
    {
        return m_speed;
    }

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    double m_latitude;
    double m_longitude;
    Nullable<double> m_altitude;
    double m_accuracy;
    Nullable<double> m_altitudeAccuracy;
    Nullable<double> m_heading;
    Nullable<double> m_speed;
};
}

#endif
