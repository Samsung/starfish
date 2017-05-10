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

#include "dom/HTMLBodyElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue onloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onload();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnload(value0);
    return ESValue();
}

static ESValue onunloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onunload();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onunloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnunload(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLBodyElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLBodyElementString = ESString::create("HTMLBodyElement");
    ESFunctionObject* HTMLBodyElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLBodyElementString, 0, true, true);
    ESObject* HTMLBodyElementPrototypeObj =
        HTMLBodyElementFunction->protoType().asESPointer()->asESObject();
    HTMLBodyElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLBodyElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLBodyElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLBodyElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* onloadString = ESString::create("onload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLBodyElementPrototypeObj, onloadString, onloadGetterFunction,
        onloadSetterFunction);

    ESString* onunloadString = ESString::create("onunload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLBodyElementPrototypeObj, onunloadString, onunloadGetterFunction,
        onunloadSetterFunction);

    return HTMLBodyElementFunction;
}

void HTMLBodyElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLBodyElement()->protoType());

    postInit(instance);
}

bool HTMLBodyElement::isHTMLBodyElement() const
{
    return true;
}
}
