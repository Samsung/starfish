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

#include "dom/DOM.h"
#include "dom/TextTrack.h"
#include "dom/TextTrackCueList.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

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

static ESValue onCueChangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    auto eventname = (((Window*)instance->globalObject()->extraPointerData()))
                         ->starFish()
                         ->staticStrings()
                         ->m_cuechange;
    return originalObj->attributeEventListener(eventname);
}

static ESValue onCueChangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    auto eventname = (((Window*)instance->globalObject()->extraPointerData()))
                         ->starFish()
                         ->staticStrings()
                         ->m_cuechange;
    if (v.isObject() ||
        (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
        originalObj->setAttributeEventListener(eventname, v);
    } else {
        originalObj->clearAttributeEventListener(eventname);
    }
    return ESValue();
}

static ESValue cuesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    TextTrackCueList* list = originalObj->cues();
    if (list) {
        return list->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue activeCuesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    TextTrackCueList* list = originalObj->activeCues();
    if (list) {
        return list->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue kindGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    return toJSString(TextTrack::kindToString(originalObj->kind()));
}

static ESValue labelGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    return toJSString(originalObj->label());
}

static ESValue languageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    return toJSString(originalObj->language());
}

static ESValue modeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    return toJSString(TextTrack::modeToString(originalObj->mode()));
}

static ESValue modeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (firstArg.isESString()) {
        originalObj->setMode(toBrowserString(firstArg.toString()));
    }
    return firstArg;
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrack);
    return toJSString(originalObj->id());
}

ESFunctionObject* bindingTextTrack(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        TextTrack, fetchData(scriptBindingInstance)->m_eventTarget);

    TextTrackFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("addCue"), false, false, false,
            ESFunctionObject::create(NULL, addCueFunction,
                                     ESString::create("addCue"), 1, false));

    TextTrackFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("removeCue"), false, false, false,
            ESFunctionObject::create(NULL, removeCueFunction,
                                     ESString::create("removeCue"), 0, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("oncuechange"), onCueChangeGetterFunction,
        onCueChangeSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("cues"), cuesGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("activeCues"), activeCuesGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("kind"), kindGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("label"), labelGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("language"), languageGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("mode"), modeGetterFunction, modeSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(),
        ESString::create("id"), idGetterFunction, nullptr);

    return TextTrackFunction;
}
}
#endif
