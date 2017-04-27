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
#include "dom/Element.h"
#include "dom/HTMLCollection.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLCollection);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLCollection);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "item",
                        "HTMLCollection", "1", "0");
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
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

static ESValue namedItemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLCollection);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "namedItem",
                        "HTMLCollection", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->namedItem(value0);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

ESFunctionObject* bindingHTMLCollection(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLCollectionString = ESString::create("HTMLCollection");
    ESFunctionObject* HTMLCollectionFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLCollectionString, 0, true, true);
    HTMLCollectionFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLCollectionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLCollectionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)
                           ->m_instance->globalObject()
                           ->objectPrototype());
    ESObject* HTMLCollectionPrototypeObj =
        HTMLCollectionFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLCollectionPrototypeObj, lengthString, lengthGetterFunction,
        nullptr);

    // Bind for functions
    ESString* itemString = ESString::create("item");
    ESFunctionObject* itemESFn =
        ESFunctionObject::create(nullptr, itemFunction, itemString, 1, false);
    HTMLCollectionPrototypeObj->defineDataProperty(itemString, true, true, true,
                                                   itemESFn);

    ESString* namedItemString = ESString::create("namedItem");
    ESFunctionObject* namedItemESFn = ESFunctionObject::create(
        nullptr, namedItemFunction, namedItemString, 1, false);
    HTMLCollectionPrototypeObj->defineDataProperty(namedItemString, true, false,
                                                   true, namedItemESFn);

    return HTMLCollectionFunction;
}
}
