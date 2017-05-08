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

#include "extra/History.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue stateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    // Declare native value (empty when type is void)
    ScriptValue result;
    result = originalObj->state();
    // Return ESValue from native value
    return result;
}

// Implement for functions
static ESValue goFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    int32_t value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toInt32();
    }
    // Call native function (nargs: 1)
    originalObj->go(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue backFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->back();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue forwardFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->forward();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue pushStateFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "pushState", "History", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg2
    Nullable<String*> value2;
    if (!arg2.isUndefinedOrNull()) {
        value2 = toBrowserString(arg2);
    }
    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Handle argument arg0
    ScriptValue value0;
    value0 = arg0;

    // Call native function (nargs: 3)
    originalObj->pushState(value0, value1, value2);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue replaceStateFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "replaceState", "History",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg2
    Nullable<String*> value2;
    if (!arg2.isUndefinedOrNull()) {
        value2 = toBrowserString(arg2);
    }
    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Handle argument arg0
    ScriptValue value0;
    value0 = arg0;

    // Call native function (nargs: 3)
    originalObj->replaceState(value0, value1, value2);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingHistory(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HistoryString = ESString::create("History");
    ESFunctionObject* HistoryFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, HistoryString, 0, true, true);
    ESObject* HistoryPrototypeObj =
        HistoryFunction->protoType().asESPointer()->asESObject();
    HistoryFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HistoryPrototypeObj->forceNonVectorHiddenClass(false);
    HistoryPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                          ->m_instance->globalObject()
                                          ->objectPrototype());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HistoryPrototypeObj, lengthString, lengthGetterFunction, nullptr);

    ESString* stateString = ESString::create("state");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HistoryPrototypeObj, stateString, stateGetterFunction, nullptr);

    // Bind for functions
    ESString* goString = ESString::create("go");
    ESFunctionObject* goESFn =
        ESFunctionObject::create(nullptr, goFunction, goString, 0, false);
    HistoryPrototypeObj->defineDataProperty(goString, true, true, true, goESFn);

    ESString* backString = ESString::create("back");
    ESFunctionObject* backESFn =
        ESFunctionObject::create(nullptr, backFunction, backString, 0, false);
    HistoryPrototypeObj->defineDataProperty(backString, true, true, true,
                                            backESFn);

    ESString* forwardString = ESString::create("forward");
    ESFunctionObject* forwardESFn = ESFunctionObject::create(
        nullptr, forwardFunction, forwardString, 0, false);
    HistoryPrototypeObj->defineDataProperty(forwardString, true, true, true,
                                            forwardESFn);

    ESString* pushStateString = ESString::create("pushState");
    ESFunctionObject* pushStateESFn = ESFunctionObject::create(
        nullptr, pushStateFunction, pushStateString, 2, false);
    HistoryPrototypeObj->defineDataProperty(pushStateString, true, true, true,
                                            pushStateESFn);

    ESString* replaceStateString = ESString::create("replaceState");
    ESFunctionObject* replaceStateESFn = ESFunctionObject::create(
        nullptr, replaceStateFunction, replaceStateString, 2, false);
    HistoryPrototypeObj->defineDataProperty(replaceStateString, true, true,
                                            true, replaceStateESFn);

    return HistoryFunction;
}
}
