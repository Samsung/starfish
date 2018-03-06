/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/modules/location/Geolocation.h"
#include "core/modules/location/Geoposition.h"
#include "core/modules/location/PositionError.h"
#include "core/dom/Document.h"

#include <EscargotPublic.h>

namespace StarFish {

using namespace Escargot;

static void geopositionCallbackFunction(Document* document, Geoposition* pos,
                                        void* data)
{
    if (data) {
        FunctionObjectRef* fn = (FunctionObjectRef*)data;
        ValueRef* a = pos->scriptValue();
        callScriptFunction(document->scriptBindingInstance(),
                           ValueRef::create(fn), &a, 1, scriptUndefined());
    }
}

static void geopositionErrorCallbackFunction(Document* document,
                                             PositionError* error, void* data)
{
    if (data) {
        FunctionObjectRef* fn = (FunctionObjectRef*)data;
        ValueRef* a = error->scriptValue();
        callScriptFunction(document->scriptBindingInstance(),
                           ValueRef::create(fn), &a, 1, scriptUndefined());
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
            state, ValueRef::create(StringRef::fromASCII("maximumAge")));
        double maximumAgeNumberDouble = maximumAge->toNumber(state);
        if (std::isnan(maximumAgeNumberDouble) || maximumAgeNumberDouble < 0) {
            maximumAgeNumber = 0;
        } else {
            maximumAgeNumber = maximumAgeNumberDouble;
        }

        ValueRef* timeout = opt->asObject()->get(
            state, ValueRef::create(StringRef::fromASCII("timeout")));
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
                ->get(state, ValueRef::create(
                                 StringRef::fromASCII("enableHighAccuracy")))
                ->toBoolean(state);
    }

    ValueRef* cb0 = argv[0];
    ValueRef* cb1 = argc >= 2 ? argv[1] : scriptUndefined();
    originalObj->getCurrentPosition(
        geopositionCallbackFunction,
        cb0->isFunction() ? cb0->asFunction() : nullptr,
        geopositionErrorCallbackFunction,
        cb1->isFunction() ? cb1->asFunction() : nullptr, enableHighAccuracy,
        timeoutNumber, maximumAgeNumber);

    return scriptUndefined();
}
}
