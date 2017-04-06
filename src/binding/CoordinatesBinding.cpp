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
#include "platform/location/Coordinates.h"

namespace StarFish {

using namespace escargot;

static ESValue latitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    return ESValue(originalObj->latitude());
}

static ESValue longitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    return ESValue(originalObj->longitude());
}

static ESValue altitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    if (originalObj->altitude()) {
        return ESValue(*originalObj->altitude());
    } else {
        return ScriptValueNull;
    }
}

static ESValue accuracyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    return ESValue(originalObj->accuracy());
}

static ESValue altitudeAccuracyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    if (originalObj->altitudeAccuracy()) {
        return ESValue(*originalObj->altitudeAccuracy());
    } else {
        return ScriptValueNull;
    }
}

static ESValue headingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    if (originalObj->heading()) {
        return ESValue(*originalObj->heading());
    } else {
        return ESValue(std::numeric_limits<double>::quiet_NaN());
    }
}

static ESValue speedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    if (originalObj->speed()) {
        return ESValue(*originalObj->speed());
    } else {
        return ScriptValueNull;
    }
}

ESFunctionObject* bindingCoordinates(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Coordinates, fetchData(scriptBindingInstance)
                                     ->m_instance->globalObject()
                                     ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("latitude"), latitudeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("longitude"), longitudeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("altitude"), altitudeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("accuracy"), accuracyGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("altitudeAccuracy"), altitudeAccuracyGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("heading"), headingGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("speed"), speedGetterFunction, nullptr);

    return CoordinatesFunction;
}
}
