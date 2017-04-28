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
#include "dom/TextTrack.h"
#include "dom/TextTrackCue.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue trackGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    // Declare native value (empty when type is void)
    TextTrack* result = nullptr;
    result = originalObj->track();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->id();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue idSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setId(value0);
    return ESValue();
}

static ESValue startTimeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->startTime();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue startTimeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    if (!std::isfinite(value0)) {
        THROW_EXCEPTION(FAILED_TO_SET_NONFINITE_PROPERTY_WHERE_EXPECTED_DOUBLE,
                        "startTime", "TextTrackCue");
    }
    originalObj->setStartTime(value0);
    return ESValue();
}

static ESValue endTimeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->endTime();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue endTimeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    if (!std::isfinite(value0)) {
        THROW_EXCEPTION(FAILED_TO_SET_NONFINITE_PROPERTY_WHERE_EXPECTED_DOUBLE,
                        "endTime", "TextTrackCue");
    }
    originalObj->setEndTime(value0);
    return ESValue();
}

static ESValue onenterGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onenter();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onenterSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnenter(value0);
    return ESValue();
}

static ESValue onexitGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onexit();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onexitSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnexit(value0);
    return ESValue();
}

ESFunctionObject* bindingTextTrackCue(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TextTrackCueString = ESString::create("TextTrackCue");
    ESFunctionObject* TextTrackCueFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, TextTrackCueString, 0, true, true);
    ESObject* TextTrackCuePrototypeObj =
        TextTrackCueFunction->protoType().asESPointer()->asESObject();
    TextTrackCueFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextTrackCuePrototypeObj->forceNonVectorHiddenClass(false);
    TextTrackCuePrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    TextTrackCueFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    // Bind for attributes
    ESString* trackString = ESString::create("track");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCuePrototypeObj, trackString, trackGetterFunction, nullptr);

    ESString* idString = ESString::create("id");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCuePrototypeObj, idString, idGetterFunction, idSetterFunction);

    ESString* startTimeString = ESString::create("startTime");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCuePrototypeObj, startTimeString, startTimeGetterFunction,
        startTimeSetterFunction);

    ESString* endTimeString = ESString::create("endTime");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCuePrototypeObj, endTimeString, endTimeGetterFunction,
        endTimeSetterFunction);

    ESString* onenterString = ESString::create("onenter");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCuePrototypeObj, onenterString, onenterGetterFunction,
        onenterSetterFunction);

    ESString* onexitString = ESString::create("onexit");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCuePrototypeObj, onexitString, onexitGetterFunction,
        onexitSetterFunction);

    return TextTrackCueFunction;
}
}
#endif
