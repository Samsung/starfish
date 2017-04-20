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
#if defined(STARFISH_ENABLE_MULTIMEDIA)
#include "StarFishConfig.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/TextTrack.h"
#include "dom/DOMException.h"
#include "dom/HTMLTrackElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue kindGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->kind();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue kindSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setKind(value0);
    return ESValue();
}

static ESValue srcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->src();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue srcSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSrc(value0);
    return ESValue();
}

static ESValue srclangGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->srclang();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue srclangSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSrclang(value0);
    return ESValue();
}

static ESValue labelGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->label();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue labelSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setLabel(value0);
    return ESValue();
}

static ESValue defaultGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->defaultAttr();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue defaultSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    bool value0;
    value0 = arg0.toBoolean();
    originalObj->setDefaultAttr(value0);
    return ESValue();
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->readyState();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue trackGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLTrackElement);
    // Declare return value (empty when void)
    TextTrack* result = nullptr;
    result = originalObj->track();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingHTMLTrackElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLTrackElementString = ESString::create("HTMLTrackElement");
    ESFunctionObject* HTMLTrackElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLTrackElementString, 1, true, true);
    HTMLTrackElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLTrackElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLTrackElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLTrackElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for constants
    ESString* NONEString = ESString::create("NONE");
    ESValue NONEValue = ESValue(0);
    HTMLTrackElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(NONEString, false, true, false, NONEValue);

    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        NONEString, false, true, false, NONEValue);

    ESString* LOADINGString = ESString::create("LOADING");
    ESValue LOADINGValue = ESValue(1);
    HTMLTrackElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(LOADINGString, false, true, false, LOADINGValue);

    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        LOADINGString, false, true, false, LOADINGValue);

    ESString* LOADEDString = ESString::create("LOADED");
    ESValue LOADEDValue = ESValue(2);
    HTMLTrackElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(LOADEDString, false, true, false, LOADEDValue);

    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        LOADEDString, false, true, false, LOADEDValue);

    ESString* ERRORString = ESString::create("ERROR");
    ESValue ERRORValue = ESValue(3);
    HTMLTrackElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ERRORString, false, true, false, ERRORValue);

    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        ERRORString, false, true, false, ERRORValue);

    // Bind for attributes
    ESString* kindString = ESString::create("kind");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        kindString, kindGetterFunction, kindSetterFunction);

    ESString* srcString = ESString::create("src");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        srcString, srcGetterFunction, srcSetterFunction);

    ESString* srclangString = ESString::create("srclang");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        srclangString, srclangGetterFunction, srclangSetterFunction);

    ESString* labelString = ESString::create("label");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        labelString, labelGetterFunction, labelSetterFunction);

    ESString* defaultString = ESString::create("default");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        defaultString, defaultGetterFunction, defaultSetterFunction);

    ESString* readyStateString = ESString::create("readyState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        readyStateString, readyStateGetterFunction, nullptr);

    ESString* trackString = ESString::create("track");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(),
        trackString, trackGetterFunction, nullptr);

    return HTMLTrackElementFunction;
}
}
#endif
