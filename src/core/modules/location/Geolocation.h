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

#ifndef __StarfishGeolocation__
#define __StarfishGeolocation__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace Starfish {

class Document;
class Geoposition;
class PositionError;

typedef void (*GeoPositionCallback)(Document*, Geoposition*, void* data);
typedef void (*GeoPositionErrorCallback)(Document*, PositionError* error,
                                         void* data);

class Geolocation : public ScriptWrappable, public DocumentHoldable {
public:
    static Geolocation* create(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isGeolocation() const override;

    virtual void getCurrentPosition(GeoPositionCallback cb, void* cbData,
                                    GeoPositionErrorCallback errorCb,
                                    void* errorCbData, bool enableHighAccuracy,
                                    int32_t timeout, int32_t maximumAge);
    virtual uint32_t watchPosition(GeoPositionCallback cb, void* cbData,
                                   GeoPositionErrorCallback errorCb,
                                   void* errorCbData, bool enableHighAccuracy,
                                   int32_t timeout, int32_t maximumAge);
    virtual void clearWatch(uint32_t watchId);
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    virtual void dispose()
    {
    }

    // CDP Emulation.setGeolocationOverride support. When an override is set,
    // the default (non-Tizen) backend delivers these coordinates to
    // getCurrentPosition/watchPosition callbacks instead of failing.
    // Process-wide (single-target headless CDP); set/cleared on the main
    // thread.
    static void setOverride(double latitude, double longitude, double accuracy);
    static void clearOverride();
    static bool hasOverride();
    static double overrideLatitude();
    static double overrideLongitude();
    static double overrideAccuracy();

protected:
    Geolocation(Document* document);
    bool getCurrentPositionPreprocessing(GeoPositionCallback cb, void* cbData,
                                         GeoPositionErrorCallback errorCb,
                                         void* errorCbData,
                                         bool enableHighAccuracy,
                                         int32_t timeout, int32_t maximumAge);
};
} // namespace Starfish

#endif
