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

#include "dom/Event.h"
#include "dom/EventTarget.h"

namespace StarFish {

using namespace escargot;

// Implement for functions
static ESValue addEventListenerFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(EventTarget);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "addEventListener",
                        "EventTarget", reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 3;
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg2
    bool value2;
    if (arg2.isUndefined()) {
        validArgCount--;
    } else {
        value2 = arg2.toBoolean();
    }
    // Handle argument arg1
    EventListener* value1 = nullptr;
    if (!arg1.isUndefinedOrNull()) {
        value1 = EventListener::toEventListener(arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 2-3)
    if (validArgCount == 2) {
        originalObj->addEventListener(value0, value1);
    } else if (validArgCount == 3) {
        originalObj->addEventListener(value0, value1, value2);
    }

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue removeEventListenerFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(EventTarget);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "removeEventListener",
                        "EventTarget", reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 3;
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg2
    bool value2;
    if (arg2.isUndefined()) {
        validArgCount--;
    } else {
        value2 = arg2.toBoolean();
    }
    // Handle argument arg1
    EventListener* value1 = nullptr;
    if (!arg1.isUndefinedOrNull()) {
        value1 = EventListener::toEventListener(arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 2-3)
    if (validArgCount == 2) {
        originalObj->removeEventListener(value0, value1);
    } else if (validArgCount == 3) {
        originalObj->removeEventListener(value0, value1, value2);
    }

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue dispatchEventFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(EventTarget);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "dispatchEvent", "EventTarget",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    bool result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    Event* value0 = nullptr;
    CHECK_TYPEOF(arg0, Event);
    value0 = (Event*)(arg0.asESPointer()->asESObject()->extraPointerData());

    // Call native function (nargs: 1)
    result = originalObj->dispatchEvent(value0);

    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingEventTarget(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* EventTargetString = ESString::create("EventTarget");
    ESFunctionObject* EventTargetFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, EventTargetString, 0, true, true);
    ESObject* EventTargetPrototypeObj =
        EventTargetFunction->protoType().asESPointer()->asESObject();
    EventTargetFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    EventTargetPrototypeObj->forceNonVectorHiddenClass(false);
    EventTargetPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                              ->m_instance->globalObject()
                                              ->objectPrototype());

    // Bind for functions
    ESString* addEventListenerString = ESString::create("addEventListener");
    ESFunctionObject* addEventListenerESFn = ESFunctionObject::create(
        nullptr, addEventListenerFunction, addEventListenerString, 2, false);
    addEventListenerESFn->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetPrototypeObj->defineDataProperty(
        addEventListenerString, true, true, true, addEventListenerESFn);

    ESString* removeEventListenerString =
        ESString::create("removeEventListener");
    ESFunctionObject* removeEventListenerESFn =
        ESFunctionObject::create(nullptr, removeEventListenerFunction,
                                 removeEventListenerString, 2, false);
    removeEventListenerESFn->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetPrototypeObj->defineDataProperty(
        removeEventListenerString, true, true, true, removeEventListenerESFn);

    ESString* dispatchEventString = ESString::create("dispatchEvent");
    ESFunctionObject* dispatchEventESFn = ESFunctionObject::create(
        nullptr, dispatchEventFunction, dispatchEventString, 1, false);
    dispatchEventESFn->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetPrototypeObj->defineDataProperty(dispatchEventString, true, true,
                                                true, dispatchEventESFn);

    return EventTargetFunction;
}
}
