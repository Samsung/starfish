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
#include "Starfish.h"
#include "core/modules/location/Geolocation.h"
#include "core/modules/location/PositionError.h"
#include "core/modules/location/Geoposition.h"
#include "core/modules/location/Coordinates.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/Document.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace Starfish {

// CDP Emulation.setGeolocationOverride state (process-wide; single-target).
static bool s_geoOverrideActive = false;
static double s_geoOverrideLat = 0;
static double s_geoOverrideLng = 0;
static double s_geoOverrideAccuracy = 0;

void Geolocation::setOverride(double latitude, double longitude,
                              double accuracy)
{
    s_geoOverrideActive = true;
    s_geoOverrideLat = latitude;
    s_geoOverrideLng = longitude;
    s_geoOverrideAccuracy = accuracy;
}

void Geolocation::clearOverride()
{
    s_geoOverrideActive = false;
}

bool Geolocation::hasOverride()
{
    return s_geoOverrideActive;
}

double Geolocation::overrideLatitude()
{
    return s_geoOverrideLat;
}

double Geolocation::overrideLongitude()
{
    return s_geoOverrideLng;
}

double Geolocation::overrideAccuracy()
{
    return s_geoOverrideAccuracy;
}

// Build a Geoposition from the active override and deliver it via the success
// callback on the message loop (async, matching the spec and the engine's
// existing idler-based delivery).
static void deliverOverridePosition(Document* document, GeoPositionCallback cb,
                                    void* cbData)
{
    Coordinates* coords = new Coordinates(
        document, s_geoOverrideLat, s_geoOverrideLng, Optional<double>(),
        s_geoOverrideAccuracy, Optional<double>(), Optional<double>(),
        Optional<double>());
    Geoposition* pos = new Geoposition(document, coords, 0);
    if (cb) {
        cb(document, pos, cbData);
    }
}

#if !defined(STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED)
Geolocation* Geolocation::create(Document* document)
{
    return new Geolocation(document);
}
#endif

Geolocation::Geolocation(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
{
}

bool Geolocation::getCurrentPositionPreprocessing(
    GeoPositionCallback cb, void* cbData, GeoPositionErrorCallback errorCb,
    void* errorCbData, bool enableHighAccuracy, int32_t timeout,
    int32_t maximumAge)
{
    if (timeout == 0) {
        m_document->webView()->messageLoop()->addIdler(
            window(),
            [](size_t, void* data, void* data2, void* data3) {
                Document* document = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(document,
                   new PositionError(document->executionContext(),
                                     PositionError::Error::TIMEOUT),
                   data3);
            },
            m_document, (void*)errorCb, errorCbData);
        return false;
    }
    return true;
}

void Geolocation::getCurrentPosition(GeoPositionCallback cb, void* cbData,
                                     GeoPositionErrorCallback errorCb,
                                     void* errorCbData, bool enableHighAccuracy,
                                     int32_t timeout, int32_t maximumAge)
{
    if (getCurrentPositionPreprocessing(cb, cbData, errorCb, errorCbData,
                                        enableHighAccuracy, timeout,
                                        maximumAge)) {
        if (s_geoOverrideActive) {
            m_document->webView()->messageLoop()->addIdler(
                window(),
                [](size_t, void* data, void* data2, void* data3) {
                    Document* document = (Document*)data;
                    GeoPositionCallback cb = (GeoPositionCallback)data2;
                    deliverOverridePosition(document, cb, data3);
                },
                m_document, (void*)cb, cbData);
            return;
        }
        m_document->webView()->messageLoop()->addIdler(
            window(),
            [](size_t, void* data, void* data2, void* data3) {
                Document* document = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(document,
                   new PositionError(
                       document->executionContext(),
                       PositionError::Error::POSITION_UNAVAILABLE),
                   data3);
            },
            m_document, (void*)errorCb, errorCbData);
    }
}

ScriptBindingInstance* Geolocation::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

uint32_t Geolocation::watchPosition(GeoPositionCallback cb, void* cbData,
                                    GeoPositionErrorCallback errorCb,
                                    void* errorCbData, bool enableHighAccuracy,
                                    int32_t timeout, int32_t maximumAge)
{
    if (getCurrentPositionPreprocessing(cb, cbData, errorCb, errorCbData,
                                        enableHighAccuracy, timeout,
                                        maximumAge)) {
        if (s_geoOverrideActive) {
            m_document->webView()->messageLoop()->addIdler(
                window(),
                [](size_t, void* data, void* data2, void* data3) {
                    Document* document = (Document*)data;
                    GeoPositionCallback cb = (GeoPositionCallback)data2;
                    deliverOverridePosition(document, cb, data3);
                },
                m_document, (void*)cb, cbData);
            return 0;
        }
        m_document->webView()->messageLoop()->addIdler(
            window(),
            [](size_t, void* data, void* data2, void* data3) {
                STARFISH_ASSERT(data != nullptr);
                STARFISH_ASSERT(data2 != nullptr);
                Document* document = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(document,
                   new PositionError(
                       document->executionContext(),
                       PositionError::Error::POSITION_UNAVAILABLE),
                   data3);
            },
            m_document, (void*)errorCb, errorCbData);
    }
    return 0;
}

void Geolocation::clearWatch(uint32_t watchId)
{
}
} // namespace Starfish
