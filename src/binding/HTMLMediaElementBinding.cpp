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
#include "extra/TimeRanges.h"
#include "dom/TextTrackList.h"
#include "dom/TextTrack.h"
#include "dom/HTMLMediaElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue srcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->currentSrc();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue networkStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->networkState();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue preloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
    TimeRanges* result = nullptr;
    result = originalObj->buffered();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->readyState();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue seekingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->seeking();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue currentTimeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "currentTime",
                        "HTMLMediaElement");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
        THROW_EXCEPTION(msg);
    }
    originalObj->setCurrentTime(value0);
    return ESValue();
}

static ESValue durationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->duration();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue pausedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->paused();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue defaultPlaybackRateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "defaultPlaybackRate",
                        "HTMLMediaElement");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
        THROW_EXCEPTION(msg);
    }
    originalObj->setDefaultPlaybackRate(value0);
    return ESValue();
}

static ESValue playbackRateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "playbackRate",
                        "HTMLMediaElement");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
        THROW_EXCEPTION(msg);
    }
    originalObj->setPlaybackRate(value0);
    return ESValue();
}

static ESValue playedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    TimeRanges* result = nullptr;
    result = originalObj->played();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue seekableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    TimeRanges* result = nullptr;
    result = originalObj->seekable();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue endedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->ended();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue autoplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
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
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "volume",
                        "HTMLMediaElement");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
        THROW_EXCEPTION(msg);
    }
    originalObj->setVolume(value0);
    return ESValue();
}

static ESValue mutedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
    TextTrackList* result = nullptr;
    result = originalObj->textTracks();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

// Implement for functions
static ESValue loadFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    // Declare native value (empty when type is void)
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
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "canPlayType",
                        "HTMLMediaElement", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
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
// Declare native value (empty when type is void)
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
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->pause();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue addTextTrackFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "addTextTrack",
                        "HTMLMediaElement", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    TextTrack* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg2
    String* value2 = String::emptyString;
    if (!arg2.isUndefinedOrNull()) {
        value2 = toBrowserString(arg2);
    }
    // Handle argument arg1
    String* value1 = String::emptyString;
    if (!arg1.isUndefinedOrNull()) {
        value1 = toBrowserString(arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Call native function (nargs: 3)
    result = originalObj->addTextTrack(value0, value1, value2);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingHTMLMediaElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLMediaElementString = ESString::create("HTMLMediaElement");
    ESFunctionObject* HTMLMediaElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLMediaElementString, 0, true, true);
    ESObject* HTMLMediaElementPrototypeObj =
        HTMLMediaElementFunction->protoType().asESPointer()->asESObject();
    HTMLMediaElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLMediaElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLMediaElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLMediaElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for constants
    ESString* NETWORK_EMPTYString = ESString::create("NETWORK_EMPTY");
    ESValue NETWORK_EMPTYValue = ESValue(0);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        NETWORK_EMPTYString, false, true, false, NETWORK_EMPTYValue);

    HTMLMediaElementFunction->defineDataProperty(
        NETWORK_EMPTYString, false, true, false, NETWORK_EMPTYValue);

    ESString* NETWORK_IDLEString = ESString::create("NETWORK_IDLE");
    ESValue NETWORK_IDLEValue = ESValue(1);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        NETWORK_IDLEString, false, true, false, NETWORK_IDLEValue);

    HTMLMediaElementFunction->defineDataProperty(
        NETWORK_IDLEString, false, true, false, NETWORK_IDLEValue);

    ESString* NETWORK_LOADINGString = ESString::create("NETWORK_LOADING");
    ESValue NETWORK_LOADINGValue = ESValue(2);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        NETWORK_LOADINGString, false, true, false, NETWORK_LOADINGValue);

    HTMLMediaElementFunction->defineDataProperty(
        NETWORK_LOADINGString, false, true, false, NETWORK_LOADINGValue);

    ESString* NETWORK_NO_SOURCEString = ESString::create("NETWORK_NO_SOURCE");
    ESValue NETWORK_NO_SOURCEValue = ESValue(3);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        NETWORK_NO_SOURCEString, false, true, false, NETWORK_NO_SOURCEValue);

    HTMLMediaElementFunction->defineDataProperty(
        NETWORK_NO_SOURCEString, false, true, false, NETWORK_NO_SOURCEValue);

    ESString* HAVE_NOTHINGString = ESString::create("HAVE_NOTHING");
    ESValue HAVE_NOTHINGValue = ESValue(0);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        HAVE_NOTHINGString, false, true, false, HAVE_NOTHINGValue);

    HTMLMediaElementFunction->defineDataProperty(
        HAVE_NOTHINGString, false, true, false, HAVE_NOTHINGValue);

    ESString* HAVE_METADATAString = ESString::create("HAVE_METADATA");
    ESValue HAVE_METADATAValue = ESValue(1);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        HAVE_METADATAString, false, true, false, HAVE_METADATAValue);

    HTMLMediaElementFunction->defineDataProperty(
        HAVE_METADATAString, false, true, false, HAVE_METADATAValue);

    ESString* HAVE_CURRENT_DATAString = ESString::create("HAVE_CURRENT_DATA");
    ESValue HAVE_CURRENT_DATAValue = ESValue(2);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        HAVE_CURRENT_DATAString, false, true, false, HAVE_CURRENT_DATAValue);

    HTMLMediaElementFunction->defineDataProperty(
        HAVE_CURRENT_DATAString, false, true, false, HAVE_CURRENT_DATAValue);

    ESString* HAVE_FUTURE_DATAString = ESString::create("HAVE_FUTURE_DATA");
    ESValue HAVE_FUTURE_DATAValue = ESValue(3);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        HAVE_FUTURE_DATAString, false, true, false, HAVE_FUTURE_DATAValue);

    HTMLMediaElementFunction->defineDataProperty(
        HAVE_FUTURE_DATAString, false, true, false, HAVE_FUTURE_DATAValue);

    ESString* HAVE_ENOUGH_DATAString = ESString::create("HAVE_ENOUGH_DATA");
    ESValue HAVE_ENOUGH_DATAValue = ESValue(4);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        HAVE_ENOUGH_DATAString, false, true, false, HAVE_ENOUGH_DATAValue);

    HTMLMediaElementFunction->defineDataProperty(
        HAVE_ENOUGH_DATAString, false, true, false, HAVE_ENOUGH_DATAValue);

    // Bind for attributes
    ESString* srcString = ESString::create("src");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, srcString, srcGetterFunction,
        srcSetterFunction);

    ESString* currentSrcString = ESString::create("currentSrc");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, currentSrcString,
        currentSrcGetterFunction, nullptr);

    ESString* networkStateString = ESString::create("networkState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, networkStateString,
        networkStateGetterFunction, nullptr);

    ESString* preloadString = ESString::create("preload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, preloadString, preloadGetterFunction,
        preloadSetterFunction);

    ESString* bufferedString = ESString::create("buffered");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, bufferedString, bufferedGetterFunction,
        nullptr);

    ESString* readyStateString = ESString::create("readyState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, readyStateString,
        readyStateGetterFunction, nullptr);

    ESString* seekingString = ESString::create("seeking");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, seekingString, seekingGetterFunction,
        nullptr);

    ESString* currentTimeString = ESString::create("currentTime");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, currentTimeString,
        currentTimeGetterFunction, currentTimeSetterFunction);

    ESString* durationString = ESString::create("duration");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, durationString, durationGetterFunction,
        nullptr);

    ESString* pausedString = ESString::create("paused");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, pausedString, pausedGetterFunction,
        nullptr);

    ESString* defaultPlaybackRateString =
        ESString::create("defaultPlaybackRate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, defaultPlaybackRateString,
        defaultPlaybackRateGetterFunction, defaultPlaybackRateSetterFunction);

    ESString* playbackRateString = ESString::create("playbackRate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, playbackRateString,
        playbackRateGetterFunction, playbackRateSetterFunction);

    ESString* playedString = ESString::create("played");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, playedString, playedGetterFunction,
        nullptr);

    ESString* seekableString = ESString::create("seekable");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, seekableString, seekableGetterFunction,
        nullptr);

    ESString* endedString = ESString::create("ended");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, endedString, endedGetterFunction,
        nullptr);

    ESString* autoplayString = ESString::create("autoplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, autoplayString, autoplayGetterFunction,
        autoplaySetterFunction);

    ESString* loopString = ESString::create("loop");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, loopString, loopGetterFunction,
        loopSetterFunction);

    ESString* controlsString = ESString::create("controls");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, controlsString, controlsGetterFunction,
        controlsSetterFunction);

    ESString* volumeString = ESString::create("volume");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, volumeString, volumeGetterFunction,
        volumeSetterFunction);

    ESString* mutedString = ESString::create("muted");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, mutedString, mutedGetterFunction,
        mutedSetterFunction);

    ESString* textTracksString = ESString::create("textTracks");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLMediaElementPrototypeObj, textTracksString,
        textTracksGetterFunction, nullptr);

    // Bind for functions
    ESString* loadString = ESString::create("load");
    ESFunctionObject* loadESFn =
        ESFunctionObject::create(nullptr, loadFunction, loadString, 0, false);
    HTMLMediaElementPrototypeObj->defineDataProperty(loadString, true, true,
                                                     true, loadESFn);

    ESString* canPlayTypeString = ESString::create("canPlayType");
    ESFunctionObject* canPlayTypeESFn = ESFunctionObject::create(
        nullptr, canPlayTypeFunction, canPlayTypeString, 1, false);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        canPlayTypeString, true, true, true, canPlayTypeESFn);

    ESString* playString = ESString::create("play");
    ESFunctionObject* playESFn =
        ESFunctionObject::create(nullptr, playFunction, playString, 0, false);
    HTMLMediaElementPrototypeObj->defineDataProperty(playString, true, true,
                                                     true, playESFn);

    ESString* pauseString = ESString::create("pause");
    ESFunctionObject* pauseESFn =
        ESFunctionObject::create(nullptr, pauseFunction, pauseString, 0, false);
    HTMLMediaElementPrototypeObj->defineDataProperty(pauseString, true, true,
                                                     true, pauseESFn);

    ESString* addTextTrackString = ESString::create("addTextTrack");
    ESFunctionObject* addTextTrackESFn = ESFunctionObject::create(
        nullptr, addTextTrackFunction, addTextTrackString, 1, false);
    HTMLMediaElementPrototypeObj->defineDataProperty(
        addTextTrackString, true, true, true, addTextTrackESFn);

    return HTMLMediaElementFunction;
}

void HTMLMediaElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLMediaElement()->protoType());

    postInit(instance);
}

bool HTMLMediaElement::isHTMLMediaElement() const
{
    return true;
}
}
#endif
