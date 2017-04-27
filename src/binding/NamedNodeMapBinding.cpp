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

#include "StarFish.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/Attr.h"
#include "dom/Document.h"
#include "dom/DOMException.h"
#include "dom/Element.h"
#include "dom/NamedNodeMap.h"
#include "platform/window/Window.h"

namespace StarFish {

using namespace escargot;

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "item",
                        "NamedNodeMap", "1", "0");
    }
    // Declare return value (empty when void)
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
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "getNamedItem", "NamedNodeMap", "1", "0");
    }
    // Declare return value (empty when void)
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
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "setNamedItem", "NamedNodeMap", "1", "0");
    }
    // Declare return value (empty when void)
    Attr* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    Attr* value0 = nullptr;
    if (arg0.isUndefinedOrNull()) {
        instance->throwError(
            ESValue(TypeError::create(ESString::create("Wrong argument"))));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        CHECK_TYPEOF(arg0, Attr);
        value0 = (Attr*)(arg0.asESPointer()->asESObject()->extraPointerData());
    }
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
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "removeNamedItem", "NamedNodeMap", "1", "0");
    }
    // Declare return value (empty when void)
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
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NamedNodeMap,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    /* 4.8.1 Interface NamedNodeMap */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NamedNodeMapFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), false, false, false,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getNamedItem"), false, false, false,
            ESFunctionObject::create(NULL, getNamedItemFunction,
                                     ESString::create("getNamedItem"), 1,
                                     false));

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("setNamedItem"), false, false, false,
            ESFunctionObject::create(NULL, setNamedItemFunction,
                                     ESString::create("setNamedItem"), 1,
                                     false));

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("removeNamedItem"), false, false, false,
            ESFunctionObject::create(NULL, removeNamedItemFunction,
                                     ESString::create("removeNamedItem"), 1,
                                     false));

    return NamedNodeMapFunction;
}
}
