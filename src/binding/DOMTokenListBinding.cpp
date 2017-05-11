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

#include "dom/DOMException.h"
#include "dom/DOMTokenList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", "0");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "item", "DOMTokenList", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
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
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "contains", "DOMTokenList",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
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

static ESValue addFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    // Declare native value (empty when type is void)
    // Handle ellipsis arguments from index0
    GCVector<String*> value0;
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    for (size_t i = 0; i < argCount; i++) {
        ESValue item = instance->currentExecutionContext()->readArgument(i);
        value0.push_back(toBrowserString(item));
    }
    // Call native function (nargs: 1)
    try {
        originalObj->add(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue removeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    // Declare native value (empty when type is void)
    // Handle ellipsis arguments from index0
    GCVector<String*> value0;
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    for (size_t i = 0; i < argCount; i++) {
        ESValue item = instance->currentExecutionContext()->readArgument(i);
        value0.push_back(toBrowserString(item));
    }
    // Call native function (nargs: 1)
    try {
        originalObj->remove(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue toggleFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "toggle", "DOMTokenList",
                        reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 2;
    // Declare native value (empty when type is void)
    bool result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    bool value1;
    if (arg1.isUndefined()) {
        validArgCount--;
    } else {
        value1 = arg1.toBoolean();
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
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
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    // Call native function (nargs: 0)
    result = originalObj->toString();

    // Return ESValue from native value
    return toJSString(result);
}

ESFunctionObject* bindingDOMTokenList(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMTokenListString = ESString::create("DOMTokenList");
    ESFunctionObject* DOMTokenListFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, DOMTokenListString, 0, true, true);
    ESObject* DOMTokenListPrototypeObj =
        DOMTokenListFunction->protoType().asESPointer()->asESObject();
    DOMTokenListFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMTokenListPrototypeObj->forceNonVectorHiddenClass(false);
    DOMTokenListPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                               ->m_instance->globalObject()
                                               ->objectPrototype());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMTokenListPrototypeObj, lengthString, lengthGetterFunction, nullptr);

    // Bind for functions
    ESString* itemString = ESString::create("item");
    ESFunctionObject* itemESFn =
        ESFunctionObject::create(nullptr, itemFunction, itemString, 1, false);
    DOMTokenListPrototypeObj->defineDataProperty(itemString, true, true, true,
                                                 itemESFn);

    ESString* containsString = ESString::create("contains");
    ESFunctionObject* containsESFn = ESFunctionObject::create(
        nullptr, containsFunction, containsString, 1, false);
    DOMTokenListPrototypeObj->defineDataProperty(containsString, true, true,
                                                 true, containsESFn);

    ESString* addString = ESString::create("add");
    ESFunctionObject* addESFn =
        ESFunctionObject::create(nullptr, addFunction, addString, 0, false);
    DOMTokenListPrototypeObj->defineDataProperty(addString, true, true, true,
                                                 addESFn);

    ESString* removeString = ESString::create("remove");
    ESFunctionObject* removeESFn = ESFunctionObject::create(
        nullptr, removeFunction, removeString, 0, false);
    DOMTokenListPrototypeObj->defineDataProperty(removeString, true, true, true,
                                                 removeESFn);

    ESString* toggleString = ESString::create("toggle");
    ESFunctionObject* toggleESFn = ESFunctionObject::create(
        nullptr, toggleFunction, toggleString, 1, false);
    DOMTokenListPrototypeObj->defineDataProperty(toggleString, true, true, true,
                                                 toggleESFn);

    ESString* toStringString = ESString::create("toString");
    ESFunctionObject* toStringESFn = ESFunctionObject::create(
        nullptr, toStringFunction, toStringString, 0, false);
    DOMTokenListPrototypeObj->defineDataProperty(toStringString, true, true,
                                                 true, toStringESFn);

    return DOMTokenListFunction;
}

void DOMTokenList::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDOMTokenList()->protoType());

    postInit(instance);
}

bool DOMTokenList::isDOMTokenList() const
{
    return true;
}
}
