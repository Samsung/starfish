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

ESFunctionObject* bindingGeoposition(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Geoposition, fetchData(scriptBindingInstance)
                                     ->m_instance->globalObject()
                                     ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        GeopositionFunction->protoType().asESPointer()->asESObject(),
        ESString::create("coords"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::GeopositionObject, Geoposition);
            return ESValue(originalObj->coords()->scriptObject());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        GeopositionFunction->protoType().asESPointer()->asESObject(),
        ESString::create("timestamp"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::GeopositionObject, Geoposition);
            return ESValue(originalObj->timestamp());
        },
        nullptr);

    return GeopositionFunction;
}
}
