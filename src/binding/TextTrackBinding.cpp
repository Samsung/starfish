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
#include "dom/TextTrack.h"
#include "dom/TextTrackCueList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue kindGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->kind();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue labelGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->label();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue languageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->language();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->id();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue modeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->mode();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue modeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setMode(value0);
    return ESValue();
}

static ESValue cuesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    TextTrackCueList* result = nullptr;
    result = originalObj->cues();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue activeCuesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare return value (empty when void)
    TextTrackCueList* result = nullptr;
    result = originalObj->activeCues();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue addCueFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    CHECK_TYPEOF(thisValue, TextTrack);
    CHECK_TYPEOF(firstArg, TextTrackCue);

    TextTrack* textTrack =
        (TextTrack*)thisValue.asESPointer()->asESObject()->extraPointerData();
    TextTrackCue* cue =
        (TextTrackCue*)firstArg.asESPointer()->asESObject()->extraPointerData();
    textTrack->addCue(cue);
    return ESValue(ESValue::ESUndefined);
}

static ESValue removeCueFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    CHECK_TYPEOF(thisValue, TextTrack);
    CHECK_TYPEOF(firstArg, TextTrackCue);

    TextTrack* textTrack =
        (TextTrack*)thisValue.asESPointer()->asESObject()->extraPointerData();
    TextTrackCue* cue =
        (TextTrackCue*)firstArg.asESPointer()->asESObject()->extraPointerData();
    textTrack->removeCue(cue);
    return ESValue(ESValue::ESUndefined);
}

static ESValue oncuechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);

    return originalObj->oncuechange();
}

static ESValue oncuechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);

    ESValue v = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOncuechange(v);

    return ESValue();
}

ESFunctionObject* bindingTextTrack(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TextTrackString = ESString::create("TextTrack");
    ESFunctionObject* TextTrackFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, TextTrackString, 0, true, true);
    TextTrackFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextTrackFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    TextTrackFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    TextTrackFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());
    ESObject* TextTrackPrototypeObj =
        TextTrackFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* kindString = ESString::create("kind");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, kindString, kindGetterFunction, nullptr);

    ESString* labelString = ESString::create("label");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, labelString, labelGetterFunction, nullptr);

    ESString* languageString = ESString::create("language");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, languageString, languageGetterFunction, nullptr);

    ESString* idString = ESString::create("id");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, idString, idGetterFunction, nullptr);

    ESString* modeString = ESString::create("mode");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, modeString, modeGetterFunction,
        modeSetterFunction);

    ESString* cuesString = ESString::create("cues");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, cuesString, cuesGetterFunction, nullptr);

    ESString* activeCuesString = ESString::create("activeCues");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, activeCuesString, activeCuesGetterFunction,
        nullptr);

    ESString* oncuechangeString = ESString::create("oncuechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackPrototypeObj, oncuechangeString, oncuechangeGetterFunction,
        oncuechangeSetterFunction);

    // Bind for functions
    ESString* addCueString = ESString::create("addCue");
    ESFunctionObject* addCueESFn = ESFunctionObject::create(
        nullptr, addCueFunction, addCueString, 1, false);
    TextTrackPrototypeObj->defineDataProperty(addCueString, true, true, true,
                                              addCueESFn);

    ESString* removeCueString = ESString::create("removeCue");
    ESFunctionObject* removeCueESFn = ESFunctionObject::create(
        nullptr, removeCueFunction, removeCueString, 1, false);
    TextTrackPrototypeObj->defineDataProperty(removeCueString, true, true, true,
                                              removeCueESFn);

    return TextTrackFunction;
}
}
#endif
