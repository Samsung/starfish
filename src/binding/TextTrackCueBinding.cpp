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
#include "ScriptBindingInstance.h"

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue startTimeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    return ESValue(originalObj->startTime());
}

static ESValue startTimeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double startTime = firstArg.toNumber();
    if (std::isnan(startTime)) {
        THROW_ILLEGAL_INVOCATION();
    }
    originalObj->setStartTime(startTime);
    return firstArg;
}

static ESValue endTimeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    return ESValue(originalObj->endTime());
}

static ESValue endTimeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double endTime = firstArg.toNumber();
    if (std::isnan(endTime)) {
        THROW_ILLEGAL_INVOCATION();
    }
    originalObj->setEndTime(endTime);
    return firstArg;
}

static ESValue trackGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    TextTrack* track = originalObj->track();
    if (track) {
        return track->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    return toJSString(originalObj->id());
}

static ESValue idSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    String* id = toBrowserString(firstArg.toString());
    originalObj->setId(id);
    return firstArg;
}

static ESValue onEnterGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    auto eventname = (((Window*)instance->globalObject()->extraPointerData()))
                         ->starFish()
                         ->staticStrings()
                         ->m_enter;
    return originalObj->attributeEventListener(eventname);
}

static ESValue onEnterSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    auto eventname = (((Window*)instance->globalObject()->extraPointerData()))
                         ->starFish()
                         ->staticStrings()
                         ->m_enter;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (v.isObject() ||
        (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
        originalObj->setAttributeEventListener(eventname, v);
    } else {
        originalObj->clearAttributeEventListener(eventname);
    }
    return ESValue();
}

static ESValue onExitGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    auto eventname = (((Window*)instance->globalObject()->extraPointerData()))
                         ->starFish()
                         ->staticStrings()
                         ->m_exit;
    return originalObj->attributeEventListener(eventname);
}

static ESValue onExitSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    auto eventname = (((Window*)instance->globalObject()->extraPointerData()))
                         ->starFish()
                         ->staticStrings()
                         ->m_exit;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (v.isObject() ||
        (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
        originalObj->setAttributeEventListener(eventname, v);
    } else {
        originalObj->clearAttributeEventListener(eventname);
    }
    return ESValue();
}

ESFunctionObject* bindingTextTrackCue(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        TextTrackCue, fetchData(scriptBindingInstance)->m_fnEventTarget);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(),
        ESString::create("startTime"), startTimeGetterFunction,
        startTimeSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(),
        ESString::create("endTime"), endTimeGetterFunction,
        endTimeSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(),
        ESString::create("track"), trackGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(),
        ESString::create("id"), idGetterFunction, idSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onenter"), onEnterGetterFunction,
        onEnterSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onexit"), onExitGetterFunction, onExitSetterFunction);

    return TextTrackCueFunction;
}
}
#endif
