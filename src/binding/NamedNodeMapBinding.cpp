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
#include "dom/Attr.h"
#include "dom/NamedNodeMap.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", "0");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "item", "NamedNodeMap", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Attr* result = nullptr;
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
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue getNamedItemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "getNamedItem", "NamedNodeMap",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Attr* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Call native function (nargs: 1)
    result = originalObj->getNamedItem(value0);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue setNamedItemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setNamedItem", "NamedNodeMap",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Attr* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    Attr* value0 = nullptr;
    CHECK_TYPEOF(arg0, Attr);
    value0 = (Attr*)(arg0.asESPointer()->asESObject()->extraPointerData());
    // Call native function (nargs: 1)
    result = originalObj->setNamedItem(value0);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue removeNamedItemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "removeNamedItem",
                        "NamedNodeMap", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Attr* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Call native function (nargs: 1)
    try {
        result = originalObj->removeNamedItem(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingNamedNodeMap(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* NamedNodeMapString = ESString::create("NamedNodeMap");
    ESFunctionObject* NamedNodeMapFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, NamedNodeMapString, 0, true, true);
    ESObject* NamedNodeMapPrototypeObj =
        NamedNodeMapFunction->protoType().asESPointer()->asESObject();
    NamedNodeMapFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    NamedNodeMapPrototypeObj->forceNonVectorHiddenClass(false);
    NamedNodeMapPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                               ->m_instance->globalObject()
                                               ->objectPrototype());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NamedNodeMapPrototypeObj, lengthString, lengthGetterFunction, nullptr);

    // Bind for functions
    ESString* itemString = ESString::create("item");
    ESFunctionObject* itemESFn =
        ESFunctionObject::create(nullptr, itemFunction, itemString, 1, false);
    NamedNodeMapPrototypeObj->defineDataProperty(itemString, true, true, true,
                                                 itemESFn);

    ESString* getNamedItemString = ESString::create("getNamedItem");
    ESFunctionObject* getNamedItemESFn = ESFunctionObject::create(
        nullptr, getNamedItemFunction, getNamedItemString, 1, false);
    NamedNodeMapPrototypeObj->defineDataProperty(getNamedItemString, true,
                                                 false, true, getNamedItemESFn);

    ESString* setNamedItemString = ESString::create("setNamedItem");
    ESFunctionObject* setNamedItemESFn = ESFunctionObject::create(
        nullptr, setNamedItemFunction, setNamedItemString, 1, false);
    NamedNodeMapPrototypeObj->defineDataProperty(setNamedItemString, true, true,
                                                 true, setNamedItemESFn);

    ESString* removeNamedItemString = ESString::create("removeNamedItem");
    ESFunctionObject* removeNamedItemESFn = ESFunctionObject::create(
        nullptr, removeNamedItemFunction, removeNamedItemString, 1, false);
    NamedNodeMapPrototypeObj->defineDataProperty(
        removeNamedItemString, true, true, true, removeNamedItemESFn);

    return NamedNodeMapFunction;
}

void NamedNodeMap::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnNamedNodeMap()->protoType());

    postInit(instance);
}

bool NamedNodeMap::isNamedNodeMap() const
{
    return true;
}
}
