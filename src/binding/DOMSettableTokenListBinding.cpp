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

#include "dom/DOMSettableTokenList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue valueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMSettableTokenList);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->value();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue valueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMSettableTokenList);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setValue(value0);
    return ESValue();
}

ESFunctionObject* bindingDOMSettableTokenList(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMSettableTokenListString =
        ESString::create("DOMSettableTokenList");
    ESFunctionObject* DOMSettableTokenListFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 DOMSettableTokenListString, 0, true, true);
    ESObject* DOMSettableTokenListPrototypeObj =
        DOMSettableTokenListFunction->protoType().asESPointer()->asESObject();
    DOMSettableTokenListFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMSettableTokenListPrototypeObj->forceNonVectorHiddenClass(false);
    DOMSettableTokenListPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMTokenList()->protoType());
    DOMSettableTokenListFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMTokenList());

    // Bind for attributes
    ESString* valueString = ESString::create("value");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMSettableTokenListPrototypeObj, valueString, valueGetterFunction,
        valueSetterFunction);

    return DOMSettableTokenListFunction;
}

void DOMSettableTokenList::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDOMSettableTokenList()->protoType());

    postInit(instance);
}

bool DOMSettableTokenList::isDOMSettableTokenList() const
{
    return true;
}
}
