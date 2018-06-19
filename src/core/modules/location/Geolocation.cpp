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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/modules/location/Geolocation.h"
#include "core/modules/location/PositionError.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/Document.h"

namespace StarFish {

#if !defined(STARFISH_TIZEN_MOBILE) && \
    !defined(STARFISH_TIZEN_WEARABLE_WIDGET) && !defined(STARFISH_TIZEN_TV)
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
