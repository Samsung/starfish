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

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class StarFish;

class Coordinates : public ScriptWrappable {
public:
    Coordinates(StarFish* starFish, double latitude, double longitude,
                double* altitude, double accuracy, double* altitudeAccuracy,
                double* heading, double* speed)
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

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isCoordinates() const
    {
        return true;
    }

    double latitude()
    {
        return m_latitude;
    }

    double longitude()
    {
        return m_longitude;
    }

    double* altitude()
    {
        return m_altitude;
    }

    double accuracy()
    {
        return m_accuracy;
    }

    double* altitudeAccuracy()
    {
        return m_altitudeAccuracy;
    }

    double* heading()
    {
        return m_heading;
    }

    double* speed()
    {
        return m_speed;
    }

protected:
    StarFish* m_starFish;
    double m_latitude;
    double m_longitude;
    double* m_altitude; // should be allcated by GC_MALLOC or null
    double m_accuracy;
    double* m_altitudeAccuracy; // should be allcated by GC_MALLOC or null
    double* m_heading;          // should be allcated by GC_MALLOC or null
    double* m_speed;            // should be allcated by GC_MALLOC or null
};
}

#endif
