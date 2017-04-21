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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarFishConfig.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/HTMLVideoElement.h"

namespace StarFish {

using namespace escargot;

static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->videoWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue videoHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->videoHeight();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue posterGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    String* result = String::emptyString;
    result = originalObj->src();

    return toJSString(result);
}

static ESValue posterSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLVideoElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSrc(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLVideoElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLVideoElement,
        fetchData(scriptBindingInstance)->fnHTMLMediaElement());

    ESString* widthString = ESString::create("width");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementFunction->protoType().asESPointer()->asESObject(),
        widthString, widthGetterFunction, widthSetterFunction);

    ESString* heightString = ESString::create("height");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementFunction->protoType().asESPointer()->asESObject(),
        heightString, heightGetterFunction, heightSetterFunction);

    ESString* videoWidthString = ESString::create("videoWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementFunction->protoType().asESPointer()->asESObject(),
        videoWidthString, videoWidthGetterFunction, nullptr);

    ESString* videoHeightString = ESString::create("videoHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementFunction->protoType().asESPointer()->asESObject(),
        videoHeightString, videoHeightGetterFunction, nullptr);

    ESString* posterString = ESString::create("poster");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLVideoElementFunction->protoType().asESPointer()->asESObject(),
        posterString, posterGetterFunction, posterSetterFunction);

    return HTMLVideoElementFunction;
}
}
#endif
