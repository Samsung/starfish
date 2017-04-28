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
#include "dom/TextTrackCueList.h"
#include "dom/TextTrackCue.h"
#include "dom/TextTrack.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue kindGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->kind();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue labelGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->label();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue languageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->language();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->id();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue modeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
    TextTrackCueList* result = nullptr;
    result = originalObj->activeCues();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue oncuechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->oncuechange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue oncuechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOncuechange(value0);
    return ESValue();
}

// Implement for functions

static ESValue addCueFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "addCue",
                        "TextTrack", "1", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    TextTrackCue* value0 = nullptr;
    CHECK_TYPEOF(arg0, TextTrackCue);
    value0 =
        (TextTrackCue*)(arg0.asESPointer()->asESObject()->extraPointerData());

    // Call native function (nargs: 1)
    originalObj->addCue(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue removeCueFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "removeCue",
                        "TextTrack", "1", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    TextTrackCue* value0 = nullptr;
    CHECK_TYPEOF(arg0, TextTrackCue);
    value0 =
        (TextTrackCue*)(arg0.asESPointer()->asESObject()->extraPointerData());

    // Call native function (nargs: 1)
    originalObj->removeCue(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingTextTrack(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TextTrackString = ESString::create("TextTrack");
    ESFunctionObject* TextTrackFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, TextTrackString, 0, true, true);
    ESObject* TextTrackPrototypeObj =
        TextTrackFunction->protoType().asESPointer()->asESObject();
    TextTrackFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextTrackPrototypeObj->forceNonVectorHiddenClass(false);
    TextTrackPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    TextTrackFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

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
