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
#if defined(STARFISH_ENABLE_MULTIMEDIA)
#include "extra/SourceBuffer.h"
#include "extra/SourceBufferList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBufferList);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue SourceBufferFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBufferList);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (v.toIndex() >= originalObj->length()) {
        return ESValue(ESValue::ESUndefined);
    } else {
        SourceBuffer* buffer = (*originalObj)[v.toIndex()];
        return buffer->scriptValue();
    }
}

// Implement for functions
ESFunctionObject* bindingSourceBufferList(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* SourceBufferListString = ESString::create("SourceBufferList");
    ESFunctionObject* SourceBufferListFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 SourceBufferListString, 0, true, true);
    ESObject* SourceBufferListPrototypeObj =
        SourceBufferListFunction->protoType().asESPointer()->asESObject();
    SourceBufferListFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    SourceBufferListPrototypeObj->forceNonVectorHiddenClass(false);
    SourceBufferListPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    SourceBufferListFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferListPrototypeObj, lengthString, lengthGetterFunction,
        nullptr);

    // Bind for functions
    ESString* SourceBufferString = ESString::create("SourceBuffer");
    ESFunctionObject* SourceBufferESFn = ESFunctionObject::create(
        nullptr, SourceBufferFunction, SourceBufferString, 1, false);
    SourceBufferListPrototypeObj->defineDataProperty(
        SourceBufferString, true, true, true, SourceBufferESFn);
    return SourceBufferListFunction;
}
}
#endif
