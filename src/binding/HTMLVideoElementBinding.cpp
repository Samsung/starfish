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

#include "dom/HTMLVideoElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->width();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue widthSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    originalObj->setWidth(value0);
    return ESValue();
}

static ESValue heightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->height();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue heightSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    originalObj->setHeight(value0);
    return ESValue();
}

static ESValue videoWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->videoWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue videoHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->videoHeight();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue posterGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->poster();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue posterSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setPoster(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLVideoElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLVideoElementString = ESString::create("HTMLVideoElement");
    ESFunctionObject* HTMLVideoElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLVideoElementString, 0, true, true);
    ESObject* HTMLVideoElementPrototypeObj =
        HTMLVideoElementFunction->protoType().asESPointer()->asESObject();
    HTMLVideoElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLVideoElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLVideoElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLMediaElement()->protoType());
    HTMLVideoElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLMediaElement());

    // Bind for attributes
    ESString* widthString = ESString::create("width");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementPrototypeObj, widthString, widthGetterFunction,
        widthSetterFunction);

    ESString* heightString = ESString::create("height");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementPrototypeObj, heightString, heightGetterFunction,
        heightSetterFunction);

    ESString* videoWidthString = ESString::create("videoWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementPrototypeObj, videoWidthString,
        videoWidthGetterFunction, nullptr);

    ESString* videoHeightString = ESString::create("videoHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementPrototypeObj, videoHeightString,
        videoHeightGetterFunction, nullptr);

    ESString* posterString = ESString::create("poster");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementPrototypeObj, posterString, posterGetterFunction,
        posterSetterFunction);

    return HTMLVideoElementFunction;
}

void HTMLVideoElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLVideoElement()->protoType());

    postInit(instance);
}

bool HTMLVideoElement::isHTMLVideoElement() const
{
    return true;
}
}
