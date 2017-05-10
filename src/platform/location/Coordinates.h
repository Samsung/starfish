/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishCoordinates__
#define __StarFishCoordinates__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class StarFish;

class Coordinates : public ScriptWrappable {
public:
    Coordinates(StarFish* starFish, double latitude, double longitude,
                Nullable<double> altitude, double accuracy,
                Nullable<double> altitudeAccuracy, Nullable<double> heading,
                Nullable<double> speed)
        : ScriptWrappable(this)
        , m_starFish(starFish)
        , m_latitude(latitude)
        , m_longitude(longitude)
        , m_altitude(altitude)
        , m_accuracy(accuracy)
        , m_altitudeAccuracy(altitudeAccuracy)
        , m_heading(heading)
        , m_speed(speed)
    {
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCoordinates() const override;

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
    StarFish* m_starFish;
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
