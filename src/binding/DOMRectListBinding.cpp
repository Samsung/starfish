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

#include "dom/DOMException.h"
#include "dom/DOMRectList.h"

namespace StarFish {

using namespace escargot;

static ESValue itemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMRectList);
    DOMRectList* domRectList =
        (DOMRectList*)thisValue.asESPointer()->asESObject()->extraPointerData();

    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    TO_INDEX_UINT32(argValue, idx);
    if (idx != INVALID_INDEX && idx < domRectList->length()) {
        DOMRect* rect = domRectList->item(idx);
        return rect->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectList);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

ESFunctionObject* bindingDOMRectList(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMRectList,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    DOMRectListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), true, true, true,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    return DOMRectListFunction;
}
}
