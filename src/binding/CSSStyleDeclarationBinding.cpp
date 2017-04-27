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
#include "dom/CSSStyleDeclaration.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CSSStyleDeclaration);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CSSStyleDeclaration);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "item",
                        "CSSStyleDeclaration", "1", "0");
    }
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
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
    return toJSString(result);
}

#ifdef STARFISH_ENABLE_TEST
static ESValue getPropertyValueFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CSSStyleDeclaration);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "getPropertyValue", "CSSStyleDeclaration", "1", buffer);
    }
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getPropertyValue(value0);

    // Return ESValue from native value
    return toJSString(result);
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue setPropertyFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CSSStyleDeclaration);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "setProperty", "CSSStyleDeclaration", "2", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    String* value1 = String::emptyString;
    if (!arg1.isUndefinedOrNull()) {
        value1 = toBrowserString(arg1);
    }
    // Handle argument arg2
    String* value2 = String::fromUTF8("");
    if (!arg2.isUndefinedOrNull()) {
        value2 = toBrowserString(arg2);
    }
    // Call native function (nargs: 3)
    try {
        originalObj->setProperty(value0, value1, value2);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}
#endif

ESFunctionObject* bindingCSSStyleDeclaration(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CSSStyleDeclarationString =
        ESString::create("CSSStyleDeclaration");
    ESFunctionObject* CSSStyleDeclarationFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 CSSStyleDeclarationString, 0, true, true);
    CSSStyleDeclarationFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CSSStyleDeclarationFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    CSSStyleDeclarationFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)
                           ->m_instance->globalObject()
                           ->objectPrototype());
    ESObject* CSSStyleDeclarationPrototypeObj =
        CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CSSStyleDeclarationPrototypeObj, lengthString, lengthGetterFunction,
        nullptr);

    // Bind for functions
    ESString* itemString = ESString::create("item");
    ESFunctionObject* itemESFn =
        ESFunctionObject::create(nullptr, itemFunction, itemString, 1, false);
    CSSStyleDeclarationPrototypeObj->defineDataProperty(itemString, true, true,
                                                        true, itemESFn);

#ifdef STARFISH_ENABLE_TEST
    ESString* getPropertyValueString = ESString::create("getPropertyValue");
    ESFunctionObject* getPropertyValueESFn = ESFunctionObject::create(
        nullptr, getPropertyValueFunction, getPropertyValueString, 1, false);
    CSSStyleDeclarationPrototypeObj->defineDataProperty(
        getPropertyValueString, true, true, true, getPropertyValueESFn);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* setPropertyString = ESString::create("setProperty");
    ESFunctionObject* setPropertyESFn = ESFunctionObject::create(
        nullptr, setPropertyFunction, setPropertyString, 2, false);
    CSSStyleDeclarationPrototypeObj->defineDataProperty(
        setPropertyString, true, true, true, setPropertyESFn);
#endif

    return CSSStyleDeclarationFunction;
}
}
