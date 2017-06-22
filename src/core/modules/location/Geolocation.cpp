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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/modules/location/Geolocation.h"
#include "core/modules/location/PositionError.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/Document.h"

namespace StarFish {

#if !defined(STARFISH_TIZEN_MOBILE) && !defined(STARFISH_TIZEN_WEARABLE)
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
        m_document->starFish()->messageLoop()->addIdler(
            m_document->browsingContext(),
            [](size_t, void* data, void* data2, void* data3) {
                Document* sf = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(sf, new PositionError(sf, PositionError::Error::TIMEOUT),
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
        m_document->starFish()->messageLoop()->addIdler(
            m_document->browsingContext(),
            [](size_t, void* data, void* data2, void* data3) {
                Document* sf = (Document*)data;
                GeoPositionErrorCallback cb = (GeoPositionErrorCallback)data2;
                cb(sf, new PositionError(
                           sf, PositionError::Error::POSITION_UNAVAILABLE),
                   data3);
            },
            m_document, (void*)errorCb, errorCbData);
    }
}

ScriptBindingInstance* Geolocation::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}
}
