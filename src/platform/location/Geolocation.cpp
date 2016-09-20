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

#include "StarFishConfig.h"
#include "Geolocation.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/profiling/Profiling.h"

namespace StarFish {

#ifndef STARFISH_TIZEN
Geolocation* Geolocation::create(StarFish* starFish)
{
    return new Geolocation(starFish);
}
#endif

Geolocation::Geolocation(StarFish* starFish)
    : ScriptWrappable(this)
    , m_starFish(starFish)
{

}

bool Geolocation::getCurrentPositionPreprocessing(GeopositionCallback cb, void* cbData, GeopositionErrorCallback errorCb, void* errorCbData, bool enableHighAccuracy, int32_t timeout, int32_t maximumAge)
{
    if (timeout == 0) {
        m_starFish->messageLoop()->addIdler([](size_t, void* data, void* data2, void* data3) {
            StarFish* sf = (StarFish*)data;
            GeopositionErrorCallback cb = (GeopositionErrorCallback)data2;
            cb(sf, new PositionError(sf, PositionError::Error::TIMEOUT), data3);
        }, m_starFish, (void*)errorCb, errorCbData);
        return false;
    }
    return true;
}

void Geolocation::getCurrentPosition(GeopositionCallback cb, void* cbData, GeopositionErrorCallback errorCb, void* errorCbData, bool enableHighAccuracy, int32_t timeout, int32_t maximumAge)
{
    if (getCurrentPositionPreprocessing(cb, cbData, errorCb, errorCbData, enableHighAccuracy, timeout, maximumAge)) {
        m_starFish->messageLoop()->addIdler([](size_t, void* data, void* data2, void* data3) {
            StarFish* sf = (StarFish*)data;
            GeopositionErrorCallback cb = (GeopositionErrorCallback)data2;
            cb(sf, new PositionError(sf, PositionError::Error::POSITION_UNAVAILABLE), data3);
        }, m_starFish, (void*)errorCb, errorCbData);
    }
}

}
