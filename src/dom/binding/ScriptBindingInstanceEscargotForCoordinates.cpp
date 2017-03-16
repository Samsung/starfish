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
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "platform/location/Geolocation.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingCoordinates(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Coordinates, fetchData(scriptBindingInstance)
                                     ->m_instance->globalObject()
                                     ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("latitude"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            return ESValue(originalObj->latitude());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("longitude"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            return ESValue(originalObj->longitude());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("altitude"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            if (originalObj->altitude()) {
                return ESValue(*originalObj->altitude());
            } else {
                return ScriptValueNull;
            }
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("accuracy"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            return ESValue(originalObj->accuracy());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("altitudeAccuracy"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            if (originalObj->altitudeAccuracy()) {
                return ESValue(*originalObj->altitudeAccuracy());
            } else {
                return ScriptValueNull;
            }
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("heading"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            if (originalObj->heading()) {
                return ESValue(*originalObj->heading());
            } else {
                return ESValue(std::numeric_limits<double>::quiet_NaN());
            }
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("speed"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::CoordinatesObject, Coordinates);
            if (originalObj->speed()) {
                return ESValue(*originalObj->speed());
            } else {
                return ScriptValueNull;
            }
        },
        nullptr);

    return CoordinatesFunction;
}
}
