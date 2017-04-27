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
#include "dom/HTMLMediaElement.h"
#include "dom/TextTrack.h"
#include "dom/TextTrackList.h"
#include "extra/TimeRanges.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue srcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->src();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue srcSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSrc(value0);
    return ESValue();
}

static ESValue currentSrcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->currentSrc();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue networkStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->networkState();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue preloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->preload();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue preloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setPreload(value0);
    return ESValue();
}

static ESValue bufferedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    TimeRanges* result = nullptr;
    result = originalObj->buffered();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->readyState();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue seekingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->seeking();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue currentTimeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    double result;
    result = originalObj->currentTime();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue currentTimeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setCurrentTime(value0);
    return ESValue();
}

static ESValue durationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    double result;
    result = originalObj->duration();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue pausedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->paused();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue defaultPlaybackRateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    double result;
    result = originalObj->defaultPlaybackRate();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue defaultPlaybackRateSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setDefaultPlaybackRate(value0);
    return ESValue();
}

static ESValue playbackRateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    double result;
    result = originalObj->playbackRate();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue playbackRateSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setPlaybackRate(value0);
    return ESValue();
}

static ESValue playedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    TimeRanges* result = nullptr;
    result = originalObj->played();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue seekableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    TimeRanges* result = nullptr;
    result = originalObj->seekable();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue endedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->ended();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue autoplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->autoplay();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue autoplaySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    bool value0;
    value0 = arg0.toBoolean();
    originalObj->setAutoplay(value0);
    return ESValue();
}

static ESValue loopGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->loop();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue loopSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    bool value0;
    value0 = arg0.toBoolean();
    originalObj->setLoop(value0);
    return ESValue();
}

static ESValue controlsGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->controls();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue controlsSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    bool value0;
    value0 = arg0.toBoolean();
    originalObj->setControls(value0);
    return ESValue();
}

static ESValue volumeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    double result;
    result = originalObj->volume();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue volumeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setVolume(value0);
    return ESValue();
}

static ESValue mutedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->muted();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue mutedSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    bool value0;
    value0 = arg0.toBoolean();
    originalObj->setMuted(value0);
    return ESValue();
}

static ESValue textTracksGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    TextTrackList* result = nullptr;
    result = originalObj->textTracks();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

extern ESValue onprogressHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onprogressHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onsuspendHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onsuspendHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onabortHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onabortHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onemptiedHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onemptiedHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onstalledHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onstalledHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onloadedmetadataHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue onloadedmetadataHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue onloadeddataHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue onloadeddataHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue onloadstartHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue onloadstartHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue oncanplayHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue oncanplayHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue oncanplaythroughHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue oncanplaythroughHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue onplayingHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onplayingHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onwaitingHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onwaitingHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onseekingHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onseekingHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onseekedHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onseekedHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onendedHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onendedHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue ondurationchangeHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue ondurationchangeHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue ontimeupdateHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue ontimeupdateHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue onplayHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onplayHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onpauseHTMLMediaElementGetterFunction(ESVMInstance* instance);

extern ESValue onpauseHTMLMediaElementSetterFunction(ESVMInstance* instance);

extern ESValue onratechangeHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue onratechangeHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

extern ESValue onvolumechangeHTMLMediaElementGetterFunction(
    ESVMInstance* instance);

extern ESValue onvolumechangeHTMLMediaElementSetterFunction(
    ESVMInstance* instance);

static ESValue addTextTrackFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (!(nd->isElement() && nd->asElement()->isHTMLElement() &&
          nd->asElement()->asHTMLElement()->isHTMLMediaElement())) {
        THROW_ILLEGAL_INVOCATION();
    }

    ESValue arg1 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg3 = instance->currentExecutionContext()->readArgument(2);
    String* kind = String::emptyString;
    String* label = String::emptyString;
    String* language = String::emptyString;

    // First Arg : kind
    if (arg1.isUndefinedOrNull() || !arg1.isESString()) {
        THROW_ILLEGAL_INVOCATION();
    }
    kind = toBrowserString(arg1.toString());
    // Second Arg : label (can be omitted)
    if (!arg2.isUndefinedOrNull() && !arg2.isESString()) {
        THROW_ILLEGAL_INVOCATION();
    }
    if (!arg2.isUndefinedOrNull()) {
        label = toBrowserString(arg2.toString());
    }
    // Third Arg : language (can be omitted)
    if (!arg3.isUndefinedOrNull() && !arg3.isESString()) {
        THROW_ILLEGAL_INVOCATION();
    }
    if (!arg3.isUndefinedOrNull()) {
        language = toBrowserString(arg3.toString());
    }

    HTMLMediaElement* element =
        originalObj->asElement()->asHTMLElement()->asHTMLMediaElement();
    TextTrack* track = element->addTextTrack(kind, label, language);
    if (!track) {
        THROW_ILLEGAL_INVOCATION();
    }
    return track->scriptValue();
}

static ESValue loadFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->load();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue canPlayTypeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "canPlayType", "NamedNodeMap", "1", "0");
    }
    // Declare return value (empty when void)
    String* result = String::emptyString;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->canPlayType(value0);

    // Return ESValue from native value
    return toJSString(result);
}

static ESValue playFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
// Declare return value (empty when void)
#ifdef USE_ES6_FEATURE
    Promise* result = nullptr;
#endif
// Call native function (nargs: 0)
#ifdef USE_ES6_FEATURE
    result = originalObj->play();
#else
    originalObj->play();
#endif

// Return ESValue from native value
#ifdef USE_ES6_FEATURE
    return result->scriptValue();
#else
    return ESValue(ESValue::ESUndefined);
#endif
}

static ESValue pauseFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->pause();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingHTMLMediaElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLMediaElementString = ESString::create("HTMLMediaElement");
    ESFunctionObject* HTMLMediaElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLMediaElementString, 1, true, true);
    HTMLMediaElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLMediaElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());
    ESObject* HTMLMediaElementObj =
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject();

    // Bind for constants
    ESString* NETWORK_EMPTYString = ESString::create("NETWORK_EMPTY");
    ESValue NETWORK_EMPTYValue = ESValue(0);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(NETWORK_EMPTYString, false, true, false,
                             NETWORK_EMPTYValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        NETWORK_EMPTYString, false, true, false, NETWORK_EMPTYValue);

    ESString* NETWORK_IDLEString = ESString::create("NETWORK_IDLE");
    ESValue NETWORK_IDLEValue = ESValue(1);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(NETWORK_IDLEString, false, true, false,
                             NETWORK_IDLEValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        NETWORK_IDLEString, false, true, false, NETWORK_IDLEValue);

    ESString* NETWORK_LOADINGString = ESString::create("NETWORK_LOADING");
    ESValue NETWORK_LOADINGValue = ESValue(2);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(NETWORK_LOADINGString, false, true, false,
                             NETWORK_LOADINGValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        NETWORK_LOADINGString, false, true, false, NETWORK_LOADINGValue);

    ESString* NETWORK_NO_SOURCEString = ESString::create("NETWORK_NO_SOURCE");
    ESValue NETWORK_NO_SOURCEValue = ESValue(3);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(NETWORK_NO_SOURCEString, false, true, false,
                             NETWORK_NO_SOURCEValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        NETWORK_NO_SOURCEString, false, true, false, NETWORK_NO_SOURCEValue);

    ESString* HAVE_NOTHINGString = ESString::create("HAVE_NOTHING");
    ESValue HAVE_NOTHINGValue = ESValue(0);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(HAVE_NOTHINGString, false, true, false,
                             HAVE_NOTHINGValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        HAVE_NOTHINGString, false, true, false, HAVE_NOTHINGValue);

    ESString* HAVE_METADATAString = ESString::create("HAVE_METADATA");
    ESValue HAVE_METADATAValue = ESValue(1);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(HAVE_METADATAString, false, true, false,
                             HAVE_METADATAValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        HAVE_METADATAString, false, true, false, HAVE_METADATAValue);

    ESString* HAVE_CURRENT_DATAString = ESString::create("HAVE_CURRENT_DATA");
    ESValue HAVE_CURRENT_DATAValue = ESValue(2);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(HAVE_CURRENT_DATAString, false, true, false,
                             HAVE_CURRENT_DATAValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        HAVE_CURRENT_DATAString, false, true, false, HAVE_CURRENT_DATAValue);

    ESString* HAVE_FUTURE_DATAString = ESString::create("HAVE_FUTURE_DATA");
    ESValue HAVE_FUTURE_DATAValue = ESValue(3);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(HAVE_FUTURE_DATAString, false, true, false,
                             HAVE_FUTURE_DATAValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        HAVE_FUTURE_DATAString, false, true, false, HAVE_FUTURE_DATAValue);

    ESString* HAVE_ENOUGH_DATAString = ESString::create("HAVE_ENOUGH_DATA");
    ESValue HAVE_ENOUGH_DATAValue = ESValue(4);
    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(HAVE_ENOUGH_DATAString, false, true, false,
                             HAVE_ENOUGH_DATAValue);

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        HAVE_ENOUGH_DATAString, false, true, false, HAVE_ENOUGH_DATAValue);

    // Bind for attributes
    ESString* srcString = ESString::create("src");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        srcString, srcGetterFunction, srcSetterFunction);

    ESString* currentSrcString = ESString::create("currentSrc");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        currentSrcString, currentSrcGetterFunction, nullptr);

    ESString* networkStateString = ESString::create("networkState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        networkStateString, networkStateGetterFunction, nullptr);

    ESString* preloadString = ESString::create("preload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        preloadString, preloadGetterFunction, preloadSetterFunction);

    ESString* bufferedString = ESString::create("buffered");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        bufferedString, bufferedGetterFunction, nullptr);

    ESString* readyStateString = ESString::create("readyState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        readyStateString, readyStateGetterFunction, nullptr);

    ESString* seekingString = ESString::create("seeking");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        seekingString, seekingGetterFunction, nullptr);

    ESString* currentTimeString = ESString::create("currentTime");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        currentTimeString, currentTimeGetterFunction,
        currentTimeSetterFunction);

    ESString* durationString = ESString::create("duration");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        durationString, durationGetterFunction, nullptr);

    ESString* pausedString = ESString::create("paused");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        pausedString, pausedGetterFunction, nullptr);

    ESString* defaultPlaybackRateString =
        ESString::create("defaultPlaybackRate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        defaultPlaybackRateString, defaultPlaybackRateGetterFunction,
        defaultPlaybackRateSetterFunction);

    ESString* playbackRateString = ESString::create("playbackRate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        playbackRateString, playbackRateGetterFunction,
        playbackRateSetterFunction);

    ESString* playedString = ESString::create("played");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        playedString, playedGetterFunction, nullptr);

    ESString* seekableString = ESString::create("seekable");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        seekableString, seekableGetterFunction, nullptr);

    ESString* endedString = ESString::create("ended");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        endedString, endedGetterFunction, nullptr);

    ESString* autoplayString = ESString::create("autoplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        autoplayString, autoplayGetterFunction, autoplaySetterFunction);

    ESString* loopString = ESString::create("loop");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        loopString, loopGetterFunction, loopSetterFunction);

    ESString* controlsString = ESString::create("controls");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        controlsString, controlsGetterFunction, controlsSetterFunction);

    ESString* volumeString = ESString::create("volume");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        volumeString, volumeGetterFunction, volumeSetterFunction);

    ESString* mutedString = ESString::create("muted");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        mutedString, mutedGetterFunction, mutedSetterFunction);

    ESString* textTracksString = ESString::create("textTracks");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        textTracksString, textTracksGetterFunction, nullptr);

    ESString* onprogressString = ESString::create("onprogress");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onprogressString, onprogressHTMLMediaElementGetterFunction,
        onprogressHTMLMediaElementSetterFunction);

    ESString* onsuspendString = ESString::create("onsuspend");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onsuspendString, onsuspendHTMLMediaElementGetterFunction,
        onsuspendHTMLMediaElementSetterFunction);

    ESString* onabortString = ESString::create("onabort");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onabortString, onabortHTMLMediaElementGetterFunction,
        onabortHTMLMediaElementSetterFunction);

    ESString* onemptiedString = ESString::create("onemptied");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onemptiedString, onemptiedHTMLMediaElementGetterFunction,
        onemptiedHTMLMediaElementSetterFunction);

    ESString* onstalledString = ESString::create("onstalled");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onstalledString, onstalledHTMLMediaElementGetterFunction,
        onstalledHTMLMediaElementSetterFunction);

    ESString* onloadedmetadataString = ESString::create("onloadedmetadata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onloadedmetadataString, onloadedmetadataHTMLMediaElementGetterFunction,
        onloadedmetadataHTMLMediaElementSetterFunction);

    ESString* onloadeddataString = ESString::create("onloadeddata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onloadeddataString, onloadeddataHTMLMediaElementGetterFunction,
        onloadeddataHTMLMediaElementSetterFunction);

    ESString* onloadstartString = ESString::create("onloadstart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onloadstartString, onloadstartHTMLMediaElementGetterFunction,
        onloadstartHTMLMediaElementSetterFunction);

    ESString* oncanplayString = ESString::create("oncanplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        oncanplayString, oncanplayHTMLMediaElementGetterFunction,
        oncanplayHTMLMediaElementSetterFunction);

    ESString* oncanplaythroughString = ESString::create("oncanplaythrough");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        oncanplaythroughString, oncanplaythroughHTMLMediaElementGetterFunction,
        oncanplaythroughHTMLMediaElementSetterFunction);

    ESString* onplayingString = ESString::create("onplaying");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onplayingString, onplayingHTMLMediaElementGetterFunction,
        onplayingHTMLMediaElementSetterFunction);

    ESString* onwaitingString = ESString::create("onwaiting");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onwaitingString, onwaitingHTMLMediaElementGetterFunction,
        onwaitingHTMLMediaElementSetterFunction);

    ESString* onseekingString = ESString::create("onseeking");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onseekingString, onseekingHTMLMediaElementGetterFunction,
        onseekingHTMLMediaElementSetterFunction);

    ESString* onseekedString = ESString::create("onseeked");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onseekedString, onseekedHTMLMediaElementGetterFunction,
        onseekedHTMLMediaElementSetterFunction);

    ESString* onendedString = ESString::create("onended");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onendedString, onendedHTMLMediaElementGetterFunction,
        onendedHTMLMediaElementSetterFunction);

    ESString* ondurationchangeString = ESString::create("ondurationchange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        ondurationchangeString, ondurationchangeHTMLMediaElementGetterFunction,
        ondurationchangeHTMLMediaElementSetterFunction);

    ESString* ontimeupdateString = ESString::create("ontimeupdate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        ontimeupdateString, ontimeupdateHTMLMediaElementGetterFunction,
        ontimeupdateHTMLMediaElementSetterFunction);

    ESString* onplayString = ESString::create("onplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onplayString, onplayHTMLMediaElementGetterFunction,
        onplayHTMLMediaElementSetterFunction);

    ESString* onpauseString = ESString::create("onpause");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onpauseString, onpauseHTMLMediaElementGetterFunction,
        onpauseHTMLMediaElementSetterFunction);

    ESString* onratechangeString = ESString::create("onratechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onratechangeString, onratechangeHTMLMediaElementGetterFunction,
        onratechangeHTMLMediaElementSetterFunction);

    ESString* onvolumechangeString = ESString::create("onvolumechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject(),
        onvolumechangeString, onvolumechangeHTMLMediaElementGetterFunction,
        onvolumechangeHTMLMediaElementSetterFunction);

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("addTextTrack"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, addTextTrackFunction,
                                 ESString::create("addTextTrack"), 3, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("load"), true, true, true,
                             ESFunctionObject::create(NULL, loadFunction,
                                                      ESString::create("load"),
                                                      0, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("canPlayType"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, canPlayTypeFunction,
                                 ESString::create("canPlayType"), 1, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("pause"), true, true, true,
                             ESFunctionObject::create(NULL, pauseFunction,
                                                      ESString::create("pause"),
                                                      0, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("play"), true, true, true,
                             ESFunctionObject::create(NULL, playFunction,
                                                      ESString::create("play"),
                                                      0, false));

    return HTMLMediaElementFunction;
}
}
#endif
