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

#include "platform/location/Coordinates.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue latitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->latitude();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue longitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->longitude();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue altitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    Nullable<double> result;
    result = originalObj->altitude();
    // Return ESValue from native value
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    double result_value = result.getValue();
    return ESValue(result_value);
}

static ESValue accuracyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->accuracy();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue altitudeAccuracyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    Nullable<double> result;
    result = originalObj->altitudeAccuracy();
    // Return ESValue from native value
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    double result_value = result.getValue();
    return ESValue(result_value);
}

static ESValue headingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    Nullable<double> result;
    result = originalObj->heading();
    // Return ESValue from native value
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    double result_value = result.getValue();
    return ESValue(result_value);
}

static ESValue speedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    // Declare native value (empty when type is void)
    Nullable<double> result;
    result = originalObj->speed();
    // Return ESValue from native value
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    double result_value = result.getValue();
    return ESValue(result_value);
}

ESFunctionObject* bindingCoordinates(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CoordinatesString = ESString::create("Coordinates");
    ESFunctionObject* CoordinatesFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, CoordinatesString, 0, true, true);
    ESObject* CoordinatesPrototypeObj =
        CoordinatesFunction->protoType().asESPointer()->asESObject();
    CoordinatesFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CoordinatesPrototypeObj->forceNonVectorHiddenClass(false);
    CoordinatesPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                              ->m_instance->globalObject()
                                              ->objectPrototype());

    // Bind for attributes
    ESString* latitudeString = ESString::create("latitude");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, latitudeString, latitudeGetterFunction,
        nullptr);

    ESString* longitudeString = ESString::create("longitude");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, longitudeString, longitudeGetterFunction,
        nullptr);

    ESString* altitudeString = ESString::create("altitude");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, altitudeString, altitudeGetterFunction,
        nullptr);

    ESString* accuracyString = ESString::create("accuracy");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, accuracyString, accuracyGetterFunction,
        nullptr);

    ESString* altitudeAccuracyString = ESString::create("altitudeAccuracy");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, altitudeAccuracyString,
        altitudeAccuracyGetterFunction, nullptr);

    ESString* headingString = ESString::create("heading");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, headingString, headingGetterFunction, nullptr);

    ESString* speedString = ESString::create("speed");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesPrototypeObj, speedString, speedGetterFunction, nullptr);

    return CoordinatesFunction;
}

void Coordinates::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnCoordinates()->protoType());

    postInit(instance);
}

bool Coordinates::isCoordinates() const
{
    return true;
}
}
