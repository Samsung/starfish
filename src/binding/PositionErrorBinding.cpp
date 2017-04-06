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
#include "platform/location/PositionError.h"

namespace StarFish {

using namespace escargot;

static ESValue codeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(PositionError);
    return ESValue(originalObj->code());
}

static ESValue messageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(PositionError);
    return ESString::create(originalObj->message());
}

ESFunctionObject* bindingPositionError(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(PositionError, fetchData(scriptBindingInstance)
                                       ->m_instance->globalObject()
                                       ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        PositionErrorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("code"), codeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        PositionErrorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("message"), messageGetterFunction, nullptr);

    PositionErrorFunction->asESObject()->defineDataProperty(
        ESString::create("PERMISSION_DENIED"), false, false, false, ESValue(1));
    PositionErrorFunction->asESObject()->defineDataProperty(
        ESString::create("POSITION_UNAVAILABLE"), false, false, false,
        ESValue(2));
    PositionErrorFunction->asESObject()->defineDataProperty(
        ESString::create("TIMEOUT"), false, false, false, ESValue(3));

    PositionErrorFunction->protoType().toObject()->defineDataProperty(
        ESString::create("PERMISSION_DENIED"), false, false, false, ESValue(1));
    PositionErrorFunction->protoType().toObject()->defineDataProperty(
        ESString::create("POSITION_UNAVAILABLE"), false, false, false,
        ESValue(2));
    PositionErrorFunction->protoType().toObject()->defineDataProperty(
        ESString::create("TIMEOUT"), false, false, false, ESValue(3));

    return PositionErrorFunction;
}
}
