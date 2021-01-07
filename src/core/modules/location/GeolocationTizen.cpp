/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/modules/location/Geolocation.h"
#if defined(STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED)
#include "core/modules/location/Geoposition.h"
#include "core/modules/location/Coordinates.h"
#include "core/modules/location/PositionError.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"
#include "Starfish.h"
#include "core/page/BrowsingContext.h"

#include <locations.h>
#include <Ecore.h>

namespace Starfish {

class GeolocationTizen;

struct LocationRequestInfoTizen {
    location_manager_h manager;
    GeolocationTizen* geolocation;
    Document* document;
    uint32_t id;
    uint32_t timeoutId;
    GeoPositionCallback cb;
    void* cbData;
    GeoPositionErrorCallback errorCb;
    void* errorCbData;
    bool enableHighAccuracy;
    bool shouldContinueRequest;
    bool shouldApplyMaxAge;
    int32_t timeout;
    int32_t maximumAge;
    bool isSingleShot;

    double altitude;
    double latitude;
    double longitude;
    double climb;
    double direction;
    double speed;
    double horizontalAccuracy;
    double verticalAccuracy;

    DOMTimeStamp timestamp;
};

class GeolocationTizen : public Geolocation {
public:
    GeolocationTizen(Document* document)
        : Geolocation(document)
    {
        m_cachedLocation.altitude = 0;
        m_cachedLocation.latitude = 0;
        m_cachedLocation.longitude = 0;
        m_cachedLocation.climb = 0;
        m_cachedLocation.direction = 0;
        m_cachedLocation.speed = 0;
        m_cachedLocation.horizontalAccuracy = 0;
        m_cachedLocation.verticalAccuracy = 0;
        m_cachedLocation.timestamp = 0;
        watchCounter = 0;
    }
    virtual void getCurrentPosition(GeoPositionCallback cb, void* cbData,
                                    GeoPositionErrorCallback errorCb,
                                    void* errorCbData, bool enableHighAccuracy,
                                    int32_t timeout, int32_t maximumAge);

    virtual uint32_t watchPosition(GeoPositionCallback cb, void* cbData,
                                   GeoPositionErrorCallback errorCb,
                                   void* errorCbData, bool enableHighAccuracy,
                                   int32_t timeout, int32_t maximumAge);

    virtual void clearWatch(uint32_t watchId);

    virtual void dispose()
    {
        auto iter = m_pendingRequest.begin();
        while (iter != m_pendingRequest.end()) {
            (*iter)->shouldContinueRequest = false;
            iter++;
        }
    }
    GCVector<LocationRequestInfoTizen*> m_pendingRequest;

    struct {
        double altitude;
        double latitude;
        double longitude;
        double climb;
        double direction;
        double speed;
        double horizontalAccuracy;
        double verticalAccuracy;
        DOMTimeStamp timestamp;
    } m_cachedLocation;

private:
    uint32_t watchCounter;
};

Geolocation* Geolocation::create(Document* d)
{
    return new GeolocationTizen(d);
}

static void sendResult(LocationRequestInfoTizen* info)
{
    Coordinates* c = new Coordinates(
        info->document, info->latitude, info->longitude,
        Nullable<double>(info->altitude), info->horizontalAccuracy,
        Nullable<double>(), Nullable<double>(info->direction),
        Nullable<double>(info->speed * 1000));

    info->cb(info->document,
             new Geoposition(info->document, c, info->timestamp), info->cbData);
    if (true == info->isSingleShot) {
        GCVector<LocationRequestInfoTizen*>& v =
            info->geolocation->m_pendingRequest;
        v.erase(std::find(v.begin(), v.end(), info));
        GC_FREE(info);
    }
}

static void handleError(int error, LocationRequestInfoTizen* info)
{
    if (error == TIZEN_ERROR_PERMISSION_DENIED) {
        info->document->webView()->messageLoop()->addIdler(
            info->document->window(),
            [](size_t, void* data, void* data2, void* data3) {
                Document* d = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(d,
                   new PositionError(d->executionContext(),
                                     PositionError::Error::PERMISSION_DENIED),
                   data3);
            },
            info->document, (void*)info->errorCb, info->errorCbData);
    } else {
        info->document->webView()->messageLoop()->addIdler(
            info->document->window(),
            [](size_t, void* data, void* data2, void* data3) {
                Document* d = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(d,
                   new PositionError(
                       d->executionContext(),
                       PositionError::Error::POSITION_UNAVAILABLE),
                   data3);
            },
            info->document, (void*)info->errorCb, info->errorCbData);
    }
    GC_FREE(info);
}

static void stopWatchingPosition(LocationRequestInfoTizen* info)
{
    STARFISH_ASSERT(info != nullptr);
    location_manager_unset_position_updated_cb(info->manager);
    location_manager_stop(info->manager);
    ecore_idler_add(
        [](void* data) -> Eina_Bool {
            location_manager_destroy((location_manager_h)data);
            return ECORE_CALLBACK_CANCEL;
        },
        (void*)info->manager);

    GCVector<LocationRequestInfoTizen*>& v =
        info->geolocation->m_pendingRequest;

    info->geolocation->m_pendingRequest.erase(
        std::find(v.begin(), v.end(), info));
    GC_FREE(info);
}

static void timerIntervalFunction(void* data)
{
    STARFISH_ASSERT(data != nullptr);
    LocationRequestInfoTizen* info = (LocationRequestInfoTizen*)data;
    if (!info->shouldContinueRequest) {
        stopWatchingPosition(info);
        return;
    }
    info->document->webView()->messageLoop()->addIdler(
        info->document->window(),
        [](size_t, void* data, void* data2, void* data3) {
            Document* d = (Document*)data;
            GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
            cb(d,
               new PositionError(d->executionContext(),
                                 PositionError::Error::TIMEOUT),
               data3);
        },
        info->document, (void*)info->errorCb, info->errorCbData);
};

void GeolocationTizen::getCurrentPosition(GeoPositionCallback cb, void* cbData,
                                          GeoPositionErrorCallback errorCb,
                                          void* errorCbData,
                                          bool enableHighAccuracy,
                                          int32_t timeout, int32_t maximumAge)
{
    if (getCurrentPositionPreprocessing(cb, cbData, errorCb, errorCbData,
                                        enableHighAccuracy, timeout,
                                        maximumAge)) {
        LocationRequestInfoTizen* info = new (NoGC) LocationRequestInfoTizen();
        info->document = document();
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
        info->isSingleShot = true;

        auto currentTimestamp = timestamp();
        if (maximumAge != 0 && (currentTimestamp - m_cachedLocation.timestamp) <
                                   (DOMTimeStamp)maximumAge) {
            // use cached location
            STARFISH_LOG_INFO("use cached location...");
            m_pendingRequest.push_back(info);
            info->altitude = m_cachedLocation.altitude;
            info->latitude = m_cachedLocation.latitude;
            info->longitude = m_cachedLocation.longitude;
            info->climb = m_cachedLocation.climb;
            info->direction = m_cachedLocation.direction;
            info->speed = m_cachedLocation.speed;
            info->horizontalAccuracy = m_cachedLocation.horizontalAccuracy;
            info->verticalAccuracy = m_cachedLocation.verticalAccuracy;
            info->timestamp = m_cachedLocation.timestamp;
            info->document->webView()->messageLoop()->addIdler(
                document()->window(),
                [](size_t, void* data) {
                    LocationRequestInfoTizen* info =
                        (LocationRequestInfoTizen*)data;
                    sendResult(info);
                },
                info);
            return;
        }

        location_method_e method = LOCATIONS_METHOD_HYBRID;
        if (enableHighAccuracy) {
            method = LOCATIONS_METHOD_GPS;
        }

        location_manager_create(method, &info->manager);
        if ((timeout / 1000) > 120) {
            STARFISH_LOG_WARN(
                "[GeolocationTizen] - timeout more than 120 seconds is ignored "
                "in Tizen");
        }
        int ret = location_manager_request_single_location(
            info->manager, 120,
            [](location_error_e error, double latitude, double longitude,
               double altitude, time_t timestamp, double speed,
               double direction, double climb, void* user_data) {
                LocationRequestInfoTizen* info =
                    (LocationRequestInfoTizen*)user_data;
                STARFISH_ASSERT(isMainThread());
                if (error) {
                    ecore_idler_add(
                        [](void* data) -> Eina_Bool {
                            location_manager_destroy((location_manager_h)data);
                            return ECORE_CALLBACK_CANCEL;
                        },
                        (void*)info->manager);

                    handleError(error, info);
                    return;
                }

                if (!info->shouldContinueRequest) {
                    ecore_idler_add(
                        [](void* data) -> Eina_Bool {
                            location_manager_destroy((location_manager_h)data);
                            return ECORE_CALLBACK_CANCEL;
                        },
                        (void*)info->manager);

                    GCVector<LocationRequestInfoTizen*>& v =
                        info->geolocation->m_pendingRequest;
                    v.erase(std::find(v.begin(), v.end(), info));
                    GC_FREE(info);
                    return;
                }

                info->document->window()->clearTimeout(info->timeoutId);

                location_accuracy_level_e level;
                location_manager_get_last_accuracy(info->manager, &level,
                                                   &info->horizontalAccuracy,
                                                   &info->verticalAccuracy);

                info->geolocation->m_cachedLocation.latitude = info->latitude =
                    latitude;
                info->geolocation->m_cachedLocation.longitude =
                    info->longitude = longitude;
                info->geolocation->m_cachedLocation.altitude = info->altitude =
                    altitude;
                info->geolocation->m_cachedLocation.speed = info->speed = speed;
                info->geolocation->m_cachedLocation.direction =
                    info->direction = direction;
                info->geolocation->m_cachedLocation.climb = info->climb = climb;
                info->geolocation->m_cachedLocation.timestamp =
                    info->timestamp = (uint64_t)timestamp * 1000L;
                info->geolocation->m_cachedLocation.horizontalAccuracy =
                    info->horizontalAccuracy;
                info->geolocation->m_cachedLocation.verticalAccuracy =
                    info->verticalAccuracy;

                info->document->webView()->messageLoop()->addIdler(
                    info->document->window(),
                    [](size_t, void* data) {
                        LocationRequestInfoTizen* info =
                            (LocationRequestInfoTizen*)data;
                        sendResult(info);
                    },
                    info);

                ecore_idler_add(
                    [](void* data) -> Eina_Bool {
                        location_manager_destroy((location_manager_h)data);
                        return ECORE_CALLBACK_CANCEL;
                    },
                    (void*)info->manager);
            },
            info);

        if (ret) {
            handleError(ret, info);
        } else {
            info->timeoutId = document()->window()->setTimeout(
                [](void* data) {
                    LocationRequestInfoTizen* info =
                        (LocationRequestInfoTizen*)data;
                    info->shouldContinueRequest = false;
                    info->document->webView()->messageLoop()->addIdler(
                        info->document->window(),
                        [](size_t, void* data, void* data2, void* data3) {
                            Document* d = (Document*)data;
                            GeoPositionErrorCallback cb =
                                (GeoPositionErrorCallback)data2;
                            cb(d,
                               new PositionError(d->executionContext(),
                                                 PositionError::Error::TIMEOUT),
                               data3);
                        },
                        info->document, (void*)info->errorCb,
                        info->errorCbData);
                },
                timeout, info);
            m_pendingRequest.push_back(info);
        }
    }
}

uint32_t GeolocationTizen::watchPosition(GeoPositionCallback cb, void* cbData,
                                         GeoPositionErrorCallback errorCb,
                                         void* errorCbData,
                                         bool enableHighAccuracy,
                                         int32_t timeout, int32_t maximumAge)
{
    LocationRequestInfoTizen* info = new (NoGC) LocationRequestInfoTizen();
    info->document = document();
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
    info->isSingleShot = false;

    // Check for cached location
    uint64_t currentTimestamp = timestamp();
    if (maximumAge != 0 && (currentTimestamp - m_cachedLocation.timestamp) <
                               (DOMTimeStamp)maximumAge) {
        info->altitude = m_cachedLocation.altitude;
        info->latitude = m_cachedLocation.latitude;
        info->longitude = m_cachedLocation.longitude;
        info->climb = m_cachedLocation.climb;
        info->direction = m_cachedLocation.direction;
        info->speed = m_cachedLocation.speed;
        info->horizontalAccuracy = m_cachedLocation.horizontalAccuracy;
        info->verticalAccuracy = m_cachedLocation.verticalAccuracy;
        info->timestamp = m_cachedLocation.timestamp;
        info->document->webView()->messageLoop()->addIdler(
            document()->window(),
            [](size_t, void* data) {
                LocationRequestInfoTizen* info =
                    (LocationRequestInfoTizen*)data;
                sendResult(info);
            },
            info);
    }

    location_method_e method = LOCATIONS_METHOD_HYBRID;
    if (enableHighAccuracy) {
        method = LOCATIONS_METHOD_GPS;
    }

    location_manager_create(method, &info->manager);
    if ((timeout / 1000) > 120) {
        STARFISH_LOG_WARN(
            "[GeolocationTizen] - timeout more than 120 seconds is ignored "
            "in Tizen");
    }

    int ret = location_manager_set_position_updated_cb(
        info->manager,
        [](double latitude, double longitude, double altitude, time_t timestamp,
           void* user_data) {
            STARFISH_ASSERT(isMainThread());
            STARFISH_ASSERT(user_data != nullptr);

            LocationRequestInfoTizen* info =
                (LocationRequestInfoTizen*)user_data;

            // Reset timer
            info->document->window()->clearInterval(info->timeoutId);
            info->timeoutId = info->document->window()->setInterval(
                timerIntervalFunction, info->timeout, info);

            location_accuracy_level_e level;
            location_manager_get_last_accuracy(info->manager, &level,
                                               &info->horizontalAccuracy,
                                               &info->verticalAccuracy);
            location_manager_get_last_velocity(info->manager, &info->climb,
                                               &info->direction, &info->speed,
                                               &timestamp);

            info->geolocation->m_cachedLocation.latitude = info->latitude =
                latitude;
            info->geolocation->m_cachedLocation.longitude = info->longitude =
                longitude;
            info->geolocation->m_cachedLocation.altitude = info->altitude =
                altitude;
            info->geolocation->m_cachedLocation.speed = info->speed;
            info->geolocation->m_cachedLocation.direction = info->direction;
            info->geolocation->m_cachedLocation.climb = info->climb;
            info->geolocation->m_cachedLocation.timestamp = info->timestamp =
                (uint64_t)timestamp * 1000L;
            info->geolocation->m_cachedLocation.horizontalAccuracy =
                info->horizontalAccuracy;
            info->geolocation->m_cachedLocation.verticalAccuracy =
                info->verticalAccuracy;

            info->document->webView()->messageLoop()->addIdler(
                info->document->window(),
                [](size_t, void* data) {
                    STARFISH_ASSERT(data != nullptr);
                    LocationRequestInfoTizen* info =
                        (LocationRequestInfoTizen*)data;
                    sendResult(info);
                },
                info);
        },
        1, info);
    if (LOCATIONS_ERROR_NONE != ret) {
        STARFISH_LOG_ERROR(
            "Failed to add location_manager_set_position_updated_cb: %d", ret);
        handleError(ret, info);
        location_manager_destroy(info->manager);
        return 0;
    }

    ret = location_manager_start(info->manager);
    if (LOCATIONS_ERROR_NONE != ret) {
        STARFISH_LOG_ERROR("Failed to start location manager: %d", ret);
        location_manager_unset_position_updated_cb(info->manager);
        handleError(ret, info);
        location_manager_destroy(info->manager);
        return 0;
    }

    // Create timer
    info->timeoutId = document()->window()->setInterval(timerIntervalFunction,
                                                        info->timeout, info);

    info->id = ++watchCounter;
    m_pendingRequest.push_back(info);
    return info->id;
}

void GeolocationTizen::clearWatch(uint32_t watchId)
{
    for (auto& v : m_pendingRequest) {
        if (v->id == watchId) {
            v->document->window()->clearInterval(v->timeoutId);
            v->shouldContinueRequest = false;
            stopWatchingPosition(v);
            return;
        }
    }
}
} // namespace Starfish

#endif
