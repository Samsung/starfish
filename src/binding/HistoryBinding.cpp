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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "extra/History.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    uint32_t result;
    result = originalObj->length();

    return ESValue(result);
}

static ESValue stateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    ScriptValue result;
    result = originalObj->state();

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
    size_t argc = instance->currentExecutionContext()->argumentCount();
    if (argc < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argc);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "pushState",
                        "History", "2", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg0
    ScriptValue value0;
    value0 = arg0;

    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Handle argument arg2
    Nullable<String*> value2;
    if (!arg2.isUndefinedOrNull()) {
        value2 = toBrowserString(arg2);
    }
    // Call native function (nargs: 3)
    originalObj->pushState(value0, value1, value2);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue replaceStateFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(History);
    size_t argc = instance->currentExecutionContext()->argumentCount();
    if (argc < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argc);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "replaceState", "History", "2", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg0
    ScriptValue value0;
    value0 = arg0;

    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Handle argument arg2
    Nullable<String*> value2;
    if (!arg2.isUndefinedOrNull()) {
        value2 = toBrowserString(arg2);
    }
    // Call native function (nargs: 3)
    originalObj->replaceState(value0, value1, value2);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingHistory(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(History, fetchData(scriptBindingInstance)
                                                 ->m_instance->globalObject()
                                                 ->objectPrototype());
    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("go"), false, false, false,
                             ESFunctionObject::create(nullptr, goFunction,
                                                      ESString::create("go"), 1,
                                                      false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("back"), false, false, false,
                             ESFunctionObject::create(nullptr, backFunction,
                                                      ESString::create("back"),
                                                      1, false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("forward"), false, false, false,
            ESFunctionObject::create(nullptr, forwardFunction,
                                     ESString::create("forward"), 1, false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("pushState"), false, false, false,
            ESFunctionObject::create(nullptr, pushStateFunction,
                                     ESString::create("pushState"), 1, false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("replaceState"), false, false, false,
            ESFunctionObject::create(nullptr, replaceStateFunction,
                                     ESString::create("pushState"), 1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HistoryFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HistoryFunction->protoType().asESPointer()->asESObject(),
        ESString::create("state"), stateGetterFunction, nullptr);
    return HistoryFunction;
}
}
