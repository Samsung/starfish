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
#include "platform/location/PositionError.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue codeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(PositionError);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->code();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue messageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(PositionError);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->message();
    // Return ESValue from native value
    return toJSString(result);
}

ESFunctionObject* bindingPositionError(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* PositionErrorString = ESString::create("PositionError");
    ESFunctionObject* PositionErrorFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 PositionErrorString, 0, true, true);
    PositionErrorFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    PositionErrorFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    PositionErrorFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)
                           ->m_instance->globalObject()
                           ->objectPrototype());

    // Bind for constants
    ESString* PERMISSION_DENIEDString = ESString::create("PERMISSION_DENIED");
    ESValue PERMISSION_DENIEDValue = ESValue(1);
    PositionErrorFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(PERMISSION_DENIEDString, false, true, false,
                             PERMISSION_DENIEDValue);

    PositionErrorFunction->asESObject()->defineDataProperty(
        PERMISSION_DENIEDString, false, true, false, PERMISSION_DENIEDValue);

    ESString* POSITION_UNAVAILABLEString =
        ESString::create("POSITION_UNAVAILABLE");
    ESValue POSITION_UNAVAILABLEValue = ESValue(2);
    PositionErrorFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(POSITION_UNAVAILABLEString, false, true, false,
                             POSITION_UNAVAILABLEValue);

    PositionErrorFunction->asESObject()->defineDataProperty(
        POSITION_UNAVAILABLEString, false, true, false,
        POSITION_UNAVAILABLEValue);

    ESString* TIMEOUTString = ESString::create("TIMEOUT");
    ESValue TIMEOUTValue = ESValue(3);
    PositionErrorFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(TIMEOUTString, false, true, false, TIMEOUTValue);

    PositionErrorFunction->asESObject()->defineDataProperty(
        TIMEOUTString, false, true, false, TIMEOUTValue);

    // Bind for attributes
    ESString* codeString = ESString::create("code");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        PositionErrorFunction->protoType().asESPointer()->asESObject(),
        codeString, codeGetterFunction, nullptr);

    ESString* messageString = ESString::create("message");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        PositionErrorFunction->protoType().asESPointer()->asESObject(),
        messageString, messageGetterFunction, nullptr);

    return PositionErrorFunction;
}
}
