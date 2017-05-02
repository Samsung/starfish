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

#include "dom/HTMLTableColElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue spanGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTableColElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->span();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue spanSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTableColElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    originalObj->setSpan(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLTableColElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLTableColElementString =
        ESString::create("HTMLTableColElement");
    ESFunctionObject* HTMLTableColElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLTableColElementString, 0, true, true);
    ESObject* HTMLTableColElementPrototypeObj =
        HTMLTableColElementFunction->protoType().asESPointer()->asESObject();
    HTMLTableColElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLTableColElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLTableColElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLTableColElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* spanString = ESString::create("span");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTableColElementPrototypeObj, spanString, spanGetterFunction,
        spanSetterFunction);

    return HTMLTableColElementFunction;
}
}
