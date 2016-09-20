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

#if defined(STARFISH_TIZEN)

#include "platform/message_loop/MessageLoop.h"
#include "platform/profiling/Profiling.h"
#include "platform/window/Window.h"

#include <locations.h>
#include <Elementary.h>

namespace StarFish {

class GeolocationTizen;

struct LocationRequestInfoTizen {
    location_manager_h manager;
    GeolocationTizen* geolocation;
    StarFish* starFish;
    uint32_t timeoutId;
    GeopositionCallback cb;
    void* cbData;
    GeopositionErrorCallback errorCb;
    void* errorCbData;
    bool enableHighAccuracy;
    bool shouldContinueRequest;
    bool shouldApplyMaxAge;
    int32_t timeout;
    int32_t maximumAge;

    double altitude;
    double latitude;
    double longitude;
    double climb;
    double direction;
    double speed;
    double horizontal;
    double vertical;
    double horizontalAccuracy;
    double verticalAccuracy;

    DOMTimeStamp timestamp;
};

class GeolocationTizen : public Geolocation {
public:
    GeolocationTizen(StarFish* starFish)
        : Geolocation(starFish)
    {
        m_cachedLocation.timestamp = 0;
    }
    virtual void getCurrentPosition(GeopositionCallback cb, void* cbData, GeopositionErrorCallback errorCb, void* errorCbData, bool enableHighAccuracy, int32_t timeout, int32_t maximumAge);
    virtual void close()
    {
        auto iter = m_pendingRequest.begin();
        while (iter != m_pendingRequest.end()) {
            (*iter)->shouldContinueRequest = false;
            iter++;
        }
    }
    std::vector<LocationRequestInfoTizen*, gc_allocator<LocationRequestInfoTizen*>> m_pendingRequest;

    struct {
        double altitude;
        double latitude;
        double longitude;
        double climb;
        double direction;
        double speed;
        double horizontal;
        double vertical;
        double horizontalAccuracy;
        double verticalAccuracy;
        DOMTimeStamp timestamp;
    } m_cachedLocation;
};

#ifdef STARFISH_TIZEN
Geolocation* Geolocation::create(StarFish* starFish)
{
    return new GeolocationTizen(starFish);
}
#endif

static void sendResult(LocationRequestInfoTizen* info)
{
    Coordinates* c = new Coordinates(info->starFish, info->latitude, info->longitude, new(GC) double(info->altitude), info->horizontalAccuracy / 2 + info->verticalAccuracy / 2, nullptr, new(GC) double(info->direction), new(GC) double(info->speed * 1000));
    info->cb(info->starFish, new Geoposition(info->starFish, c, info->timestamp), info->cbData);

    std::vector<LocationRequestInfoTizen*, gc_allocator<LocationRequestInfoTizen*>>& v = info->geolocation->m_pendingRequest;
    v.erase(std::find(v.begin(), v.end(), info));
    GC_FREE(info);
}

static void handleError(int error, LocationRequestInfoTizen* info)
{
    if (error == TIZEN_ERROR_PERMISSION_DENIED) {
        info->starFish->messageLoop()->addIdler([](size_t, void* data, void* data2, void* data3) {
            StarFish* sf = (StarFish*)data;
            GeopositionErrorCallback cb = (GeopositionErrorCallback)data2;
            cb(sf, new PositionError(sf, PositionError::Error::PERMISSION_DENIED), data3);
        }, info->starFish, (void*)info->errorCb, info->errorCbData);
    } else {
        info->starFish->messageLoop()->addIdler([](size_t, void* data, void* data2, void* data3) {
            StarFish* sf = (StarFish*)data;
            GeopositionErrorCallback cb = (GeopositionErrorCallback)data2;
            cb(sf, new PositionError(sf, PositionError::Error::POSITION_UNAVAILABLE), data3);
        }, info->starFish, (void*)info->errorCb, info->errorCbData);
    }
    GC_FREE(info);
}

void GeolocationTizen::getCurrentPosition(GeopositionCallback cb, void* cbData, GeopositionErrorCallback errorCb, void* errorCbData, bool enableHighAccuracy, int32_t timeout, int32_t maximumAge)
{
    if (getCurrentPositionPreprocessing(cb, cbData, errorCb, errorCbData, enableHighAccuracy, timeout, maximumAge)) {

        LocationRequestInfoTizen* info = new(NoGC) LocationRequestInfoTizen();
        info->starFish = m_starFish;
        info->geolocation = this;
        info->shouldContinueRequest = true;
        info->shouldApplyMaxAge = true;
        info->cb = cb;
        info->cbData = cbData;
        info->errorCb = errorCb;
        info->errorCbData = errorCbData;
        info->enableHighAccuracy = enableHighAccuracy;
        info->timeout = timeout;
        info->maximumAge = maximumAge;

        auto currentTimestamp = timestamp();
        if (maximumAge != 0 && (currentTimestamp - m_cachedLocation.timestamp) < (DOMTimeStamp)maximumAge) {
            // use cached location
            STARFISH_LOG_INFO("use cached location...");
            m_pendingRequest.push_back(info);
            info->altitude = m_cachedLocation.altitude;
            info->latitude = m_cachedLocation.latitude;
            info->longitude = m_cachedLocation.longitude;
            info->climb = m_cachedLocation.climb;
            info->direction = m_cachedLocation.direction;
            info->speed = m_cachedLocation.speed;
            info->horizontal = m_cachedLocation.horizontal;
            info->vertical = m_cachedLocation.vertical;
            info->horizontalAccuracy = m_cachedLocation.horizontalAccuracy;
            info->verticalAccuracy = m_cachedLocation.verticalAccuracy;
            info->timestamp = m_cachedLocation.timestamp;
            info->starFish->messageLoop()->addIdler([](size_t, void* data) {
                LocationRequestInfoTizen* info = (LocationRequestInfoTizen*)data;
                sendResult(info);
            }, info);
            return;
        }


        location_method_e method = LOCATIONS_METHOD_HYBRID;
        if (enableHighAccuracy) {
            method = LOCATIONS_METHOD_GPS;
        }

        location_manager_create(method, &info->manager);
        if ((timeout / 1000) > 120) {
            STARFISH_LOG_WARN("[GeolocationTizen] - timeout more than 120 seconds is ignored in Tizen");
        }
        int ret = location_manager_request_single_location(info->manager, 120, [](location_error_e error, double latitude, double longitude, double altitude, time_t timestamp, double speed, double direction, double climb, void *user_data) {
            LocationRequestInfoTizen* info = (LocationRequestInfoTizen*)user_data;
            if (error) {
                ecore_idler_add([](void* data) -> Eina_Bool {
                    location_manager_destroy((location_manager_h)data);
                    return ECORE_CALLBACK_CANCEL;
                }, (void*)info->manager);

                handleError(error, info);
                return;
            }

            if (!info->shouldContinueRequest) {
                ecore_idler_add([](void* data) -> Eina_Bool {
                    location_manager_destroy((location_manager_h)data);
                    return ECORE_CALLBACK_CANCEL;
                }, (void*)info->manager);

                std::vector<LocationRequestInfoTizen*, gc_allocator<LocationRequestInfoTizen*>>& v = info->geolocation->m_pendingRequest;
                v.erase(std::find(v.begin(), v.end(), info));
                GC_FREE(info);
                return;
            }

            info->starFish->window()->clearTimeout(info->timeoutId);

            info->geolocation->m_cachedLocation.latitude = info->latitude = latitude;
            info->geolocation->m_cachedLocation.longitude = info->longitude = longitude;
            info->geolocation->m_cachedLocation.altitude = info->altitude = altitude;
            info->geolocation->m_cachedLocation.speed = info->speed = speed;
            info->geolocation->m_cachedLocation.direction = info->direction = direction;
            info->geolocation->m_cachedLocation.climb = info->climb = climb;
            info->geolocation->m_cachedLocation.timestamp = info->timestamp = (uint64_t)timestamp * 1000L;

            location_accuracy_level_e level;
            location_manager_get_last_accuracy(info->manager, &level, &info->horizontalAccuracy, &info->verticalAccuracy);

            info->starFish->messageLoop()->addIdler([](size_t, void* data) {
                LocationRequestInfoTizen* info = (LocationRequestInfoTizen*)data;
                sendResult(info);
            }, info);

            ecore_idler_add([](void* data) -> Eina_Bool {
                location_manager_destroy((location_manager_h)data);
                return ECORE_CALLBACK_CANCEL;
            }, (void*)info->manager);
        } , info);

        if (ret) {
            handleError(ret, info);
        } else {
            info->timeoutId = m_starFish->window()->setTimeout([](Window*, void* data) {
                LocationRequestInfoTizen* info = (LocationRequestInfoTizen*)data;
                info->shouldContinueRequest = false;
                info->starFish->messageLoop()->addIdler([](size_t, void* data, void* data2, void* data3) {
                    StarFish* sf = (StarFish*)data;
                    GeopositionErrorCallback cb = (GeopositionErrorCallback)data2;
                    cb(sf, new PositionError(sf, PositionError::Error::TIMEOUT), data3);
                }, info->starFish, (void*)info->errorCb, info->errorCbData);
            }, timeout, info);
            m_pendingRequest.push_back(info);
        }
    }
}

}

#endif
