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

#include "platform/location/Geolocation.h"

namespace StarFish {

using namespace escargot;

// Implement for functions
extern ESValue getCurrentPositionGeolocationFunction(ESVMInstance* instance);

ESFunctionObject* bindingGeolocation(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* GeolocationString = ESString::create("Geolocation");
    ESFunctionObject* GeolocationFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, GeolocationString, 0, true, true);
    ESObject* GeolocationPrototypeObj =
        GeolocationFunction->protoType().asESPointer()->asESObject();
    GeolocationFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    GeolocationPrototypeObj->forceNonVectorHiddenClass(false);
    GeolocationPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                              ->m_instance->globalObject()
                                              ->objectPrototype());

    // Bind for functions
    ESString* getCurrentPositionString = ESString::create("getCurrentPosition");
    ESFunctionObject* getCurrentPositionGeolocationESFn =
        ESFunctionObject::create(nullptr, getCurrentPositionGeolocationFunction,
                                 getCurrentPositionString, 1, false);
    GeolocationPrototypeObj->defineDataProperty(
        getCurrentPositionString, true, true, true,
        getCurrentPositionGeolocationESFn);

    return GeolocationFunction;
}

void Geolocation::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnGeolocation()->protoType());

    postInit(instance);
}

bool Geolocation::isGeolocation() const
{
    return true;
}
}
