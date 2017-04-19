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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "platform/location/Coordinates.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue latitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    double v = originalObj->latitude();
    return ESValue(v);
}

static ESValue longitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    double v = originalObj->longitude();
    return ESValue(v);
}

static ESValue altitudeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    Nullable<double> v = originalObj->altitude();
    if (v.hasValue()) {
        return ESValue(v.getValue());
    }
    return ESValue(ESValue::ESNull);
}

static ESValue accuracyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    double v = originalObj->accuracy();
    return ESValue(v);
}

static ESValue altitudeAccuracyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    Nullable<double> v = originalObj->altitudeAccuracy();
    if (v.hasValue()) {
        return ESValue(v.getValue());
    }
    return ESValue(ESValue::ESNull);
}

static ESValue headingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    Nullable<double> v = originalObj->heading();
    if (v.hasValue()) {
        return ESValue(v.getValue());
    }
    return ESValue(ESValue::ESNull);
}

static ESValue speedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Coordinates);
    Nullable<double> v = originalObj->speed();
    if (v.hasValue()) {
        return ESValue(v.getValue());
    }
    return ESValue(ESValue::ESNull);
}

ESFunctionObject* bindingCoordinates(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CoordinatesString = ESString::create("Coordinates");
    ESFunctionObject* CoordinatesFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, CoordinatesString, 1, true, true);

    CoordinatesFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);

    CoordinatesFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);

    CoordinatesFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());

    // Bind for attributes
    ESString* latitudeString = ESString::create("latitude");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        latitudeString, latitudeGetterFunction, nullptr);

    ESString* longitudeString = ESString::create("longitude");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        longitudeString, longitudeGetterFunction, nullptr);

    ESString* altitudeString = ESString::create("altitude");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        altitudeString, altitudeGetterFunction, nullptr);

    ESString* accuracyString = ESString::create("accuracy");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        accuracyString, accuracyGetterFunction, nullptr);

    ESString* altitudeAccuracyString = ESString::create("altitudeAccuracy");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        altitudeAccuracyString, altitudeAccuracyGetterFunction, nullptr);

    ESString* headingString = ESString::create("heading");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        headingString, headingGetterFunction, nullptr);

    ESString* speedString = ESString::create("speed");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(),
        speedString, speedGetterFunction, nullptr);

    return CoordinatesFunction;
}
}
