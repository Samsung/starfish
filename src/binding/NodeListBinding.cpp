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
#include "dom/Node.h"
#include "dom/NodeList.h"

namespace StarFish {

using namespace escargot;

static ESValue lengthFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NodeList);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

// Implement for functions
static ESValue itemFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(NodeList);
    // Class item getter by index
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "item",
                        "NodeList", "1", "0");
    }
    // Declare native value (empty when type is void)
    Node* result = nullptr;
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

ESFunctionObject* bindingNodeList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NodeList, fetchData(scriptBindingInstance)
                                                  ->m_instance->globalObject()
                                                  ->objectPrototype());

    NodeListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), false, false, false,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthFunction, nullptr);

    return NodeListFunction;
}
}
