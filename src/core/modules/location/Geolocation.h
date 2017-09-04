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
