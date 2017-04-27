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
#include "dom/DOMTokenList.h"

namespace StarFish {

using namespace escargot;

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "item",
                        "DOMTokenList", "1", "0");
    }
    // Declare return value (empty when void)
    Nullable<String*> result = String::emptyString;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    uint32_t idx = arg0.toIndex();
    if (idx == ESValue::ESInvalidIndexValue) {
        double __number = arg0.toNumber();
        if (__number < 0) {
            return ESValue(ESValue::ESNull);
        }
        idx = std::isnan(__number) ? 0 : (uint32_t)__number;
    }
    result = originalObj->item(idx);
    // Return ESValue from native value
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    String* result_value = result.getValue();
    return toJSString(result_value);
}

static ESValue containsFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    bool result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->contains(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // Return ESValue from native value
    return ESValue(result);
}

extern ESValue addDOMTokenListFunction(ESVMInstance* instance);

static ESValue removeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    try {
        for (size_t i = 0; i < argCount; i++) {
            ESValue arg = instance->currentExecutionContext()->readArgument(i);
            String* value = toBrowserString(arg);
            originalObj->remove(value);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue toggleFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    size_t validArgCount = 2;
    // Declare return value (empty when void)
    bool result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    bool value1;
    if (arg1.isUndefinedOrNull()) {
        validArgCount--;
    } else {
        value1 = arg1.toBoolean();
    }
    // Call native function (nargs: 1-2)
    try {
        if (validArgCount == 1) {
            result = originalObj->toggle(value0);
        } else if (validArgCount == 2) {
            result = originalObj->toggle(value0, value1);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue toStringFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    // Call native function (nargs: 0)
    result = originalObj->toString();

    // Return ESValue from native value
    return toJSString(result);
}

// https://dom.spec.whatwg.org/#interface-domtokenlist
ESFunctionObject* bindingDOMTokenList(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMTokenList,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    /* 7.1 Interface DOMTokenList */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMTokenListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), false, false, false,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("contains"), false, false, false,
            ESFunctionObject::create(NULL, containsFunction,
                                     ESString::create("contains"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("add"), false, false, false,
            ESFunctionObject::create(NULL, addDOMTokenListFunction,
                                     ESString::create("add"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), false, false, false,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("toggle"), false, false, false,
            ESFunctionObject::create(NULL, toggleFunction,
                                     ESString::create("toggle"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("toString"), true, false, true,
            ESFunctionObject::create(NULL, toStringFunction,
                                     ESString::create("toString"), 1, false));

    return DOMTokenListFunction;
}
}
