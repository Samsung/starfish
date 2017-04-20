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
#include "dom/HTMLElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue dirGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->dir();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue dirSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setDir(value0);
    return ESValue();
}

static ESValue offsetWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare return value (empty when void)
    int32_t result;
    result = originalObj->offsetWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue offsetHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare return value (empty when void)
    int32_t result;
    result = originalObj->offsetHeight();
    // Return ESValue from native value
    return ESValue(result);
}

extern ESValue onclickHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onclickHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onerrorHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onerrorHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onfocusHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onfocusHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onkeydownHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onkeydownHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onkeyupHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onkeyupHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onloadHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onloadHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onmouseoverHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue onmouseoverHTMLElementSetterFunction(ESVMInstance* instance);

// Implement for functions
static ESValue clickFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->click();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue focusFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->focus();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingHTMLElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLElementString = ESString::create("HTMLElement");
    ESFunctionObject* HTMLElementFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, HTMLElementString, 1, true, true);
    HTMLElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLElementFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnElement()->protoType());
    HTMLElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnElement());
    ESObject* HTMLElementObj =
        HTMLElementFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* dirString = ESString::create("dir");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), dirString,
        dirGetterFunction, dirSetterFunction);

    ESString* offsetWidthString = ESString::create("offsetWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        offsetWidthString, offsetWidthGetterFunction, nullptr);

    ESString* offsetHeightString = ESString::create("offsetHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        offsetHeightString, offsetHeightGetterFunction, nullptr);

    ESString* onclickString = ESString::create("onclick");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onclickString, onclickHTMLElementGetterFunction,
        onclickHTMLElementSetterFunction);

    ESString* onerrorString = ESString::create("onerror");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onerrorString, onerrorHTMLElementGetterFunction,
        onerrorHTMLElementSetterFunction);

    ESString* onfocusString = ESString::create("onfocus");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onfocusString, onfocusHTMLElementGetterFunction,
        onfocusHTMLElementSetterFunction);

    ESString* onkeydownString = ESString::create("onkeydown");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onkeydownString, onkeydownHTMLElementGetterFunction,
        onkeydownHTMLElementSetterFunction);

    ESString* onkeyupString = ESString::create("onkeyup");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onkeyupString, onkeyupHTMLElementGetterFunction,
        onkeyupHTMLElementSetterFunction);

    ESString* onloadString = ESString::create("onload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onloadString, onloadHTMLElementGetterFunction,
        onloadHTMLElementSetterFunction);

    ESString* onmouseoverString = ESString::create("onmouseover");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        onmouseoverString, onmouseoverHTMLElementGetterFunction,
        onmouseoverHTMLElementSetterFunction);

    // Bind for functions
    ESString* clickString = ESString::create("click");
    ESFunctionObject* clickESFn =
        ESFunctionObject::create(nullptr, clickFunction, clickString, 0, false);
    HTMLElementObj->defineDataProperty(clickString, true, true, true,
                                       clickESFn);

    ESString* focusString = ESString::create("focus");
    ESFunctionObject* focusESFn =
        ESFunctionObject::create(nullptr, focusFunction, focusString, 0, false);
    HTMLElementObj->defineDataProperty(focusString, true, true, true,
                                       focusESFn);

    return HTMLElementFunction;
}
}
