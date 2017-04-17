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

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue offsetWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    int32_t result;
    result = originalObj->offsetWidth();

    return ESValue(result);
}

static ESValue offsetHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    int32_t result;
    result = originalObj->offsetHeight();

    return ESValue(result);
}

extern ESValue dirHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue dirHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onclickHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onclickHTMLElementSetterFunction(ESVMInstance* instance);

static ESValue clickFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    originalObj->click();
    return ESValue(ESValue::ESUndefined);
}

extern ESValue onmouseoverHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onmouseoverHTMLElementSetterFunction(ESVMInstance* instance);

static ESValue mouseoverFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* obj = originalObj;
    String* eventType = obj->document()
                            ->window()
                            ->starFish()
                            ->staticStrings()
                            ->m_mouseover.localName();
    Event* e = new Event(eventType, EventInit(true, true));
    obj->dispatchEvent(e);
    return ESValue(ESValue::ESUndefined);
}

extern ESValue onloadHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onloadHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onunloadHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onunloadHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onkeydownHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onkeydownHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onkeyupHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onkeyupHTMLElementSetterFunction(ESVMInstance* instance);

extern ESValue onfocusHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onfocusHTMLElementSetterFunction(ESVMInstance* instance);

static ESValue focusFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    originalObj->focus();
    return ESValue(ESValue::ESUndefined);
}

extern ESValue onerrorHTMLElementGetterFunction(ESVMInstance* instance);
extern ESValue onerrorHTMLElementSetterFunction(ESVMInstance* instance);

ESFunctionObject* bindingHTMLElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLElement, fetchData(scriptBindingInstance)->fnElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("dir"), dirHTMLElementGetterFunction,
        dirHTMLElementSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetWidth"), offsetWidthGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetHeight"), offsetHeightGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onclick"), onclickHTMLElementGetterFunction,
        onclickHTMLElementSetterFunction);

    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("click"), true, true, true,
                             ESFunctionObject::create(NULL, clickFunction,
                                                      ESString::create("click"),
                                                      1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onmouseover"), onmouseoverHTMLElementGetterFunction,
        onmouseoverHTMLElementSetterFunction);

    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("mouseover"), true, true, true,
            ESFunctionObject::create(NULL, mouseoverFunction,
                                     ESString::create("mouseover"), 1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onload"), onloadHTMLElementGetterFunction,
        onloadHTMLElementSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onunload"), onunloadHTMLElementGetterFunction,
        onunloadHTMLElementSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeydown"), onkeydownHTMLElementGetterFunction,
        onkeydownHTMLElementSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeyup"), onkeyupHTMLElementGetterFunction,
        onkeyupHTMLElementSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onfocus"), onfocusHTMLElementGetterFunction,
        onfocusHTMLElementSetterFunction);

    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("focus"), true, true, true,
                             ESFunctionObject::create(NULL, focusFunction,
                                                      ESString::create("focus"),
                                                      1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onerror"), onerrorHTMLElementGetterFunction,
        onerrorHTMLElementSetterFunction);

    return HTMLElementFunction;
}
}
