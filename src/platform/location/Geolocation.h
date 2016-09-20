/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishGeolocation__
#define __StarFishGeolocation__

#include "dom/binding/ScriptWrappable.h"
#include "platform/profiling/Profiling.h"

namespace StarFish {

class StarFish;

class Coordinates : public ScriptWrappable {
public:
    Coordinates(StarFish* starFish, double latitude, double longitude, double* altitude, double accuracy, double* altitudeAccuracy, double* heading, double* speed)
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

    virtual Type type()
    {
        return ScriptWrappable::Type::CoordinatesObject;
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
    double* m_heading; // should be allcated by GC_MALLOC or null
    double* m_speed; // should be allcated by GC_MALLOC or null
};

class Geoposition : public ScriptWrappable {
public:
    Geoposition(StarFish* starFish, Coordinates* c, DOMTimeStamp timestamp)
        : ScriptWrappable(this)
        , m_starFish(starFish)
        , m_coords(c)
        , m_timestamp(timestamp)
    {
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    Coordinates* coords()
    {
        return m_coords;
    }

    const DOMTimeStamp& timestamp()
    {
        return m_timestamp;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::GeopositionObject;
    }


protected:
    StarFish* m_starFish;
    Coordinates* m_coords;
    DOMTimeStamp m_timestamp;
};

class PositionError : public ScriptWrappable {
public:
    enum Error {
        PERMISSION_DENIED = 1,
        POSITION_UNAVAILABLE = 2,
        TIMEOUT = 3
    };

    PositionError(StarFish* starFish, Error code)
        : ScriptWrappable(this)
        , m_starFish(starFish)
        , m_code(code)
    {
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    Error code()
    {
        return m_code;
    }

    const char* message()
    {
        if (m_code == 1) {
            return "Permission denied";
        } else if (m_code == 2) {
            return "Position unavailable";
        } else if (m_code == 3) {
            return "Timeout expired";
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::PositionErrorObject;
    }


protected:
    StarFish* m_starFish;
    Error m_code;
};

typedef void (*GeopositionCallback)(StarFish*, Geoposition*, void* data);
typedef void (*GeopositionErrorCallback)(StarFish*, PositionError* error, void* data);

class Geolocation : public ScriptWrappable {
public:
    static Geolocation* create(StarFish* starFish);

    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::GeolocationObject;
    }

    virtual void getCurrentPosition(GeopositionCallback cb, void* cbData, GeopositionErrorCallback errorCb, void* errorCbData, bool enableHighAccuracy, int32_t timeout, int32_t maximumAge);
    virtual void close() { }
protected:
    Geolocation(StarFish* starFish);
    bool getCurrentPositionPreprocessing(GeopositionCallback cb, void* cbData, GeopositionErrorCallback errorCb, void* errorCbData, bool enableHighAccuracy, int32_t timeout, int32_t maximumAge);
    StarFish* m_starFish;
};

}

#endif
