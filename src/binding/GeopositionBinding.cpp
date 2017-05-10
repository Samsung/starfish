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
#include "platform/location/Geoposition.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue coordsGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Geoposition);
    // Declare native value (empty when type is void)
    Coordinates* result = nullptr;
    result = originalObj->coords();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue timestampGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Geoposition);
    // Declare native value (empty when type is void)
    uint64_t result;
    result = originalObj->timestamp();
    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingGeoposition(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* GeopositionString = ESString::create("Geoposition");
    ESFunctionObject* GeopositionFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, GeopositionString, 0, true, true);
    ESObject* GeopositionPrototypeObj =
        GeopositionFunction->protoType().asESPointer()->asESObject();
    GeopositionFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    GeopositionPrototypeObj->forceNonVectorHiddenClass(false);
    GeopositionPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                              ->m_instance->globalObject()
                                              ->objectPrototype());

    // Bind for attributes
    ESString* coordsString = ESString::create("coords");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        GeopositionPrototypeObj, coordsString, coordsGetterFunction, nullptr);

    ESString* timestampString = ESString::create("timestamp");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        GeopositionPrototypeObj, timestampString, timestampGetterFunction,
        nullptr);

    return GeopositionFunction;
}

void Geoposition::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnGeoposition()->protoType());

    postInit(instance);
}

bool Geoposition::isGeoposition() const
{
    return true;
}
}
