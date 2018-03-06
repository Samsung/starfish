/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishGeolocation__
#define __StarFishGeolocation__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

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
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    virtual void dispose()
    {
    }

protected:
    Geolocation(Document* document);
    bool getCurrentPositionPreprocessing(GeoPositionCallback cb, void* cbData,
                                         GeoPositionErrorCallback errorCb,
                                         void* errorCbData,
                                         bool enableHighAccuracy,
                                         int32_t timeout, int32_t maximumAge);
};
}

#endif
