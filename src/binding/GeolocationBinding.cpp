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
#include "ScriptBindingInstance.h"

#include "dom/DOM.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "platform/location/Geolocation.h"
#include "platform/location/Geoposition.h"
#include "platform/location/PositionError.h"

namespace StarFish {

using namespace escargot;

static void geopositionCallbackFunction(StarFish* starfish, Geoposition* pos,
                                        void* data)
{
    if (data) {
        ESFunctionObject* fn = (ESFunctionObject*)data;
        ESValue a = pos->scriptValue();
        callScriptFunction(fn, &a, 1, ESValue());
    }
}

static void geopositionErrorCallbackFunction(StarFish* starfish,
                                             PositionError* error, void* data)
{
    if (data) {
        ESFunctionObject* fn = (ESFunctionObject*)data;
        ESValue a = error->scriptValue();
        callScriptFunction(fn, &a, 1, ESValue());
    }
}

static ESValue getCurrentPositionFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Geolocation);

    ESValue opt = instance->currentExecutionContext()->readArgument(2);
    int32_t maximumAgeNumber = 0;
    int32_t timeoutNumber = std::numeric_limits<int32_t>::max();
    bool enableHighAccuracy = false;
    if (opt.isObject()) {
        ESValue maximumAge =
            opt.asObject()->get(ESString::create("maximumAge"));
        double maximumAgeNumberDouble = maximumAge.toNumber();
        if (std::isnan(maximumAgeNumberDouble) || maximumAgeNumberDouble < 0) {
            maximumAgeNumber = 0;
        } else {
            maximumAgeNumber = maximumAgeNumberDouble;
        }

        ESValue timeout = opt.asObject()->get(ESString::create("timeout"));
        double timeoutNumberDouble = timeout.toNumber();
        if (std::isnan(timeoutNumberDouble)) {
            timeoutNumber = std::numeric_limits<int32_t>::max();
        } else if (timeoutNumberDouble < 0) {
            timeoutNumber = 0;
        } else {
            timeoutNumber = timeoutNumberDouble;
        }

        enableHighAccuracy = opt.asObject()
                                 ->get(ESString::create("enableHighAccuracy"))
                                 .toBoolean();
    }

    ESValue cb0 = instance->currentExecutionContext()->readArgument(0);
    ESValue cb1 = instance->currentExecutionContext()->readArgument(1);
    originalObj->getCurrentPosition(
        geopositionCallbackFunction,
        cb0.isFunction() ? cb0.asFunction() : nullptr,
        geopositionErrorCallbackFunction,
        cb1.isFunction() ? cb1.asFunction() : nullptr, enableHighAccuracy,
        timeoutNumber, maximumAgeNumber);

    return ESValue();
}

ESFunctionObject* bindingGeolocation(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Geolocation, fetchData(scriptBindingInstance)
                                     ->m_instance->globalObject()
                                     ->objectPrototype());

    GeolocationFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getCurrentPosition"), true, true, true,
            ESFunctionObject::create(nullptr, getCurrentPositionFunction,
                                     ESString::create("getCurrentPosition"),
                                     1));
    return GeolocationFunction;
}
}
