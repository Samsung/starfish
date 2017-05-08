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

#include "dom/HTMLImageElement.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue htmlimageelementConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW,
                        "HTMLImageElement");
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 2;
    bool argCounting = true;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    uint32_t value1;
    if (argCounting && arg1.isUndefined()) {
        validArgCount--;
    } else {
        argCounting = false;
        value1 = arg1.toUint32();
    }
    // Handle argument arg0
    uint32_t value0;
    if (argCounting && arg0.isUndefined()) {
        validArgCount--;
    } else {
        argCounting = false;
        value0 = arg0.toUint32();
    }
    HTMLImageElement* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 0-2)
    if (validArgCount == 0) {
        result = new HTMLImageElement(callWith);
    } else if (validArgCount == 1) {
        result = new HTMLImageElement(callWith, value0);
    } else if (validArgCount == 2) {
        result = new HTMLImageElement(callWith, value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
static ESValue srcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLImageElement);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->src();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue srcSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLImageElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSrc(value0);
    return ESValue();
}

static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLImageElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->width();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue widthSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLImageElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    originalObj->setWidth(value0);
    return ESValue();
}

static ESValue heightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLImageElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->height();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue heightSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLImageElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    originalObj->setHeight(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLImageElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLImageElementString = ESString::create("HTMLImageElement");
    ESFunctionObject* HTMLImageElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLImageElementString, 0, true, true);
    ESObject* HTMLImageElementPrototypeObj =
        HTMLImageElementFunction->protoType().asESPointer()->asESObject();
    HTMLImageElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLImageElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLImageElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLImageElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* srcString = ESString::create("src");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementPrototypeObj, srcString, srcGetterFunction,
        srcSetterFunction);

    ESString* widthString = ESString::create("width");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementPrototypeObj, widthString, widthGetterFunction,
        widthSetterFunction);

    ESString* heightString = ESString::create("height");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementPrototypeObj, heightString, heightGetterFunction,
        heightSetterFunction);

    return HTMLImageElementFunction;
}

ESFunctionObject* bindingImage(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* ImageString = ESString::create("Image");

    ESFunctionObject* ImageFunction = ESFunctionObject::create(
        nullptr, htmlimageelementConstructor, ImageString, 2, true, true);

    ImageFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    ImageFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    ImageFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLImageElement()->protoType());
    ImageFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLImageElement());
    return ImageFunction;
}
}
