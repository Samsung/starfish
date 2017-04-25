/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "binding/ScriptWrappable.h"

namespace StarFish {

class StarFish;
class Geoposition;
class PositionError;

typedef void (*GeoPositionCallback)(StarFish*, Geoposition*, void* data);
typedef void (*GeoPositionErrorCallback)(StarFish*, PositionError* error,
                                         void* data);

class Geolocation : public ScriptWrappable {
public:
    static Geolocation* create(StarFish* starFish);

    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void init(ScriptBindingInstance* instance) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnGeolocation()->protoType());
    }

    virtual bool isGeolocation() const override
    {
        return true;
    }

    virtual void getCurrentPosition(GeoPositionCallback cb, void* cbData,
                                    GeoPositionErrorCallback errorCb,
                                    void* errorCbData, bool enableHighAccuracy,
                                    int32_t timeout, int32_t maximumAge);
    virtual void close()
    {
    }

protected:
    Geolocation(StarFish* starFish);
    bool getCurrentPositionPreprocessing(GeoPositionCallback cb, void* cbData,
                                         GeoPositionErrorCallback errorCb,
                                         void* errorCbData,
                                         bool enableHighAccuracy,
                                         int32_t timeout, int32_t maximumAge);
    StarFish* m_starFish;
};
}

#endif
