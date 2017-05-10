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

#include "dom/HTMLStyleElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLStyleElement);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->type();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue typeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLStyleElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setType(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLStyleElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLStyleElementString = ESString::create("HTMLStyleElement");
    ESFunctionObject* HTMLStyleElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLStyleElementString, 0, true, true);
    ESObject* HTMLStyleElementPrototypeObj =
        HTMLStyleElementFunction->protoType().asESPointer()->asESObject();
    HTMLStyleElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLStyleElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLStyleElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLStyleElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* typeString = ESString::create("type");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLStyleElementPrototypeObj, typeString, typeGetterFunction,
        typeSetterFunction);

    return HTMLStyleElementFunction;
}

void HTMLStyleElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLStyleElement()->protoType());

    postInit(instance);
}

bool HTMLStyleElement::isHTMLStyleElement() const
{
    return true;
}
}
