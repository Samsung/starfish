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

#include "dom/HTMLTableElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTableElement);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->width();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue widthSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTableElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setWidth(value0);
    return ESValue();
}

static ESValue bgColorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTableElement);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->bgColor();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue bgColorSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTableElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isNull()) {
        value0 = toBrowserString(arg0);
    }
    originalObj->setBgColor(value0);
    return ESValue();
}

// Implement for functions
ESFunctionObject* bindingHTMLTableElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLTableElementString = ESString::create("HTMLTableElement");
    ESFunctionObject* HTMLTableElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLTableElementString, 0, true, true);
    ESObject* HTMLTableElementPrototypeObj =
        HTMLTableElementFunction->protoType().asESPointer()->asESObject();
    HTMLTableElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLTableElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLTableElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLTableElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* widthString = ESString::create("width");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTableElementPrototypeObj, widthString, widthGetterFunction,
        widthSetterFunction);

    ESString* bgColorString = ESString::create("bgColor");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTableElementPrototypeObj, bgColorString, bgColorGetterFunction,
        bgColorSetterFunction);

    // Bind for functions
    return HTMLTableElementFunction;
}
}
