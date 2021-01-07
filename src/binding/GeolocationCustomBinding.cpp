/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/modules/location/Geoposition.h"
#include "core/modules/location/PositionError.h"
#include "core/dom/Document.h"

#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

static void geopositionCallbackFunction(Document* document, Geoposition* pos,
                                        void* data)
{
    if (data) {
        ObjectRef* callable = (ObjectRef*)data;
        ValueRef* a = pos->scriptValue();
        callScriptFunction(document->scriptBindingInstance(),
                           ValueRef::create(callable), &a, 1,
                           scriptUndefined());
    }
}

static void geopositionErrorCallbackFunction(Document* document,
                                             PositionError* error, void* data)
{
    if (data) {
        ObjectRef* callable = (ObjectRef*)data;
        ValueRef* a = error->scriptValue();
        callScriptFunction(document->scriptBindingInstance(),
                           ValueRef::create(callable), &a, 1,
                           scriptUndefined());
    }
}

ValueRef* getCurrentPositionGeolocationFunction(ExecutionStateRef* state,
                                                ValueRef* thisValue,
                                                size_t argc, ValueRef** argv,
                                                bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(Geolocation);

    ValueRef* opt = argc >= 3 ? argv[2] : scriptUndefined();
    int32_t maximumAgeNumber = 0;
    int32_t timeoutNumber = std::numeric_limits<int32_t>::max();
    bool enableHighAccuracy = false;
    if (opt->isObject()) {
        ValueRef* maximumAge = opt->asObject()->get(
            state, StringRef::createFromASCII("maximumAge"));
        double maximumAgeNumberDouble = maximumAge->toNumber(state);
        if (std::isnan(maximumAgeNumberDouble) || maximumAgeNumberDouble < 0) {
            maximumAgeNumber = 0;
        } else {
            maximumAgeNumber = maximumAgeNumberDouble;
        }

        ValueRef* timeout =
            opt->asObject()->get(state, StringRef::createFromASCII("timeout"));
        double timeoutNumberDouble = timeout->toNumber(state);
        if (std::isnan(timeoutNumberDouble)) {
            timeoutNumber = std::numeric_limits<int32_t>::max();
        } else if (timeoutNumberDouble < 0) {
            timeoutNumber = 0;
        } else {
            timeoutNumber = timeoutNumberDouble;
        }

        enableHighAccuracy =
            opt->asObject()
                ->get(state, StringRef::createFromASCII("enableHighAccuracy"))
                ->toBoolean(state);
    }

    ValueRef* cb0 = argv[0];
    ValueRef* cb1 = argc >= 2 ? argv[1] : scriptUndefined();
    originalObj->getCurrentPosition(
        geopositionCallbackFunction,
        cb0->isCallable() ? cb0->asObject() : nullptr,
        geopositionErrorCallbackFunction,
        cb1->isCallable() ? cb1->asObject() : nullptr, enableHighAccuracy,
        timeoutNumber, maximumAgeNumber);

    return scriptUndefined();
}

ValueRef* watchPositionGeolocationFunction(ExecutionStateRef* state,
                                           ValueRef* thisValue, size_t argc,
                                           ValueRef** argv,
                                           bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    GENERATE_THIS_AND_CHECK_TYPE(Geolocation);

    ValueRef* opt = argc >= 3 ? argv[2] : scriptUndefined();
    STARFISH_ASSERT(opt != nullptr);

    int32_t maximumAgeNumber = 0;
    int32_t timeoutNumber = std::numeric_limits<int32_t>::max();
    bool enableHighAccuracy = false;
    if (opt->isObject()) {
        ValueRef* maximumAge = opt->asObject()->get(
            state, StringRef::createFromASCII("maximumAge"));
        double maximumAgeNumberDouble = maximumAge->toNumber(state);
        if (std::isnan(maximumAgeNumberDouble) || maximumAgeNumberDouble < 0) {
            maximumAgeNumber = 0;
        } else {
            maximumAgeNumber = maximumAgeNumberDouble;
        }

        ValueRef* timeout =
            opt->asObject()->get(state, StringRef::createFromASCII("timeout"));
        double timeoutNumberDouble = timeout->toNumber(state);
        if (std::isnan(timeoutNumberDouble)) {
            timeoutNumber = std::numeric_limits<int32_t>::max();
        } else if (timeoutNumberDouble < 0) {
            timeoutNumber = 0;
        } else {
            timeoutNumber = timeoutNumberDouble;
        }

        enableHighAccuracy =
            opt->asObject()
                ->get(state, StringRef::createFromASCII("enableHighAccuracy"))
                ->toBoolean(state);
    }

    ValueRef* cb0 = argv[0];
    STARFISH_ASSERT(cb0 != nullptr);

    ValueRef* cb1 = argc >= 2 ? argv[1] : scriptUndefined();
    STARFISH_ASSERT(cb1 != nullptr);

    uint32_t result = originalObj->watchPosition(
        geopositionCallbackFunction,
        cb0->isCallable() ? cb0->asObject() : nullptr,
        geopositionErrorCallbackFunction,
        cb1->isCallable() ? cb1->asObject() : nullptr, enableHighAccuracy,
        timeoutNumber, maximumAgeNumber);

    return ValueRef::create(result);
}

ValueRef* clearWatchGeolocationFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    GENERATE_THIS_AND_CHECK_TYPE(Geolocation);
    uint32_t watchId = 0;
    if (argc > 0 && !argv[0]->isUndefinedOrNull()) {
        watchId = argv[0]->toUint32(state);
        originalObj->clearWatch(watchId);
    }
    return scriptUndefined();
}
} // namespace Starfish
