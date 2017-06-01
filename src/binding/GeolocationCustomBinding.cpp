/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
