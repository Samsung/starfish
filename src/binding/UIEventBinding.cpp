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
#include "dom/UIEvent.h"

namespace StarFish {

using namespace escargot;

extern UIEventInit toUIEventInitFromESValue(ESVMInstance* instance,
                                            ESValue& from);
extern ESValue toESValueFromUIEventInit(ESVMInstance* instance,
                                        UIEventInit& from);

// Implement for constructor
static ESValue uieventConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "UIEvent");
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARGS_NOT_ENOUGH, "UIEvent",
                        "1", buffer);
    }
    size_t validArgCount = 2;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    UIEventInit value1;
    if (arg1.isUndefinedOrNull()) {
        validArgCount--;
    } else {
        value1 = toUIEventInitFromESValue(instance, arg1);
    }
    UIEvent* result = nullptr;
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = new UIEvent(value0);
    } else if (validArgCount == 2) {
        result = new UIEvent(value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
ESFunctionObject* bindingUIEvent(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* UIEventString = ESString::create("UIEvent");
    ESFunctionObject* UIEventFunction = ESFunctionObject::create(
        nullptr, uieventConstructor, UIEventString, 1, true, true);
    UIEventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    UIEventFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    UIEventFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnEvent()->protoType());
    UIEventFunction->set__proto__(fetchData(scriptBindingInstance)->fnEvent());
    ESObject* UIEventPrototypeObj =
        UIEventFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    return UIEventFunction;
}
}
