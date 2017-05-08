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

#include "dom/MouseEvent.h"

namespace StarFish {

using namespace escargot;

extern MouseEventInit toMouseEventInitFromESValue(ESVMInstance* instance,
                                                  ESValue& from);
extern ESValue toESValueFromMouseEventInit(ESVMInstance* instance,
                                           MouseEventInit& from);

// Implement for constructor
static ESValue mouseeventConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "MouseEvent");
        THROW_EXCEPTION(msg);
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "parseFromString",
                        "MouseEvent", reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 2;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    MouseEventInit value1;
    if (arg1.isUndefined()) {
        validArgCount--;
    } else {
        value1 = toMouseEventInitFromESValue(instance, arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    MouseEvent* result = nullptr;
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = new MouseEvent(value0);
    } else if (validArgCount == 2) {
        result = new MouseEvent(value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
// Implement for functions
ESFunctionObject* bindingMouseEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* MouseEventString = ESString::create("MouseEvent");
    ESFunctionObject* MouseEventFunction = ESFunctionObject::create(
        nullptr, mouseeventConstructor, MouseEventString, 1, true, true);
    ESObject* MouseEventPrototypeObj =
        MouseEventFunction->protoType().asESPointer()->asESObject();
    MouseEventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    MouseEventPrototypeObj->forceNonVectorHiddenClass(false);
    MouseEventPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnUIEvent()->protoType());
    MouseEventFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnUIEvent());

    // Bind for attributes
    // Bind for functions
    return MouseEventFunction;
}
}
