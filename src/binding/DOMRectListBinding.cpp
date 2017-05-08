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

#include "dom/DOMRect.h"
#include "dom/DOMRectList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectList);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectList);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", "0");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "item", "DOMRectList", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    DOMRect* result = nullptr;
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

ESFunctionObject* bindingDOMRectList(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMRectListString = ESString::create("DOMRectList");
    ESFunctionObject* DOMRectListFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, DOMRectListString, 0, true, true);
    ESObject* DOMRectListPrototypeObj =
        DOMRectListFunction->protoType().asESPointer()->asESObject();
    DOMRectListFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMRectListPrototypeObj->forceNonVectorHiddenClass(false);
    DOMRectListPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                              ->m_instance->globalObject()
                                              ->objectPrototype());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectListPrototypeObj, lengthString, lengthGetterFunction, nullptr);

    // Bind for functions
    ESString* itemString = ESString::create("item");
    ESFunctionObject* itemESFn =
        ESFunctionObject::create(nullptr, itemFunction, itemString, 1, false);
    DOMRectListPrototypeObj->defineDataProperty(itemString, true, true, true,
                                                itemESFn);

    return DOMRectListFunction;
}
}
