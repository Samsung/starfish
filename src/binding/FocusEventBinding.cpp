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

#include "dom/EventTarget.h"
#include "dom/FocusEvent.h"

namespace StarFish {

using namespace escargot;

extern FocusEventInit toFocusEventInitFromESValue(ESVMInstance* instance,
                                                  ESValue& from);
extern ESValue toESValueFromFocusEventInit(ESVMInstance* instance,
                                           FocusEventInit& from);

// Implement for constructor
static ESValue focuseventConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "FocusEvent");
        THROW_EXCEPTION(msg);
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "parseFromString",
                        "FocusEvent", reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 2;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    FocusEventInit value1;
    if (arg1.isUndefined()) {
        validArgCount--;
    } else {
        value1 = toFocusEventInitFromESValue(instance, arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    FocusEvent* result = nullptr;
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = new FocusEvent(value0);
    } else if (validArgCount == 2) {
        result = new FocusEvent(value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
static ESValue relatedTargetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(FocusEvent);
    // Declare native value (empty when type is void)
    EventTarget* result = nullptr;
    result = originalObj->relatedTarget();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

ESFunctionObject* bindingFocusEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* FocusEventString = ESString::create("FocusEvent");
    ESFunctionObject* FocusEventFunction = ESFunctionObject::create(
        nullptr, focuseventConstructor, FocusEventString, 1, true, true);
    ESObject* FocusEventPrototypeObj =
        FocusEventFunction->protoType().asESPointer()->asESObject();
    FocusEventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    FocusEventPrototypeObj->forceNonVectorHiddenClass(false);
    FocusEventPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnUIEvent()->protoType());
    FocusEventFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnUIEvent());

    // Bind for attributes
    ESString* relatedTargetString = ESString::create("relatedTarget");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        FocusEventPrototypeObj, relatedTargetString,
        relatedTargetGetterFunction, nullptr);

    return FocusEventFunction;
}

void FocusEvent::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnFocusEvent()->protoType());

    postInit(instance);
}

bool FocusEvent::isFocusEvent() const
{
    return true;
}
}
