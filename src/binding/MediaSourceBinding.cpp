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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "extra/MediaSource.h"

namespace StarFish {

using namespace escargot;

static ESValue mediaSourceFunction(ESVMInstance* instance)
{
    MediaSource* b = new MediaSource(fetchDocument(instance));
    return b->scriptValue();
}

static ESValue addSourceBufferFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

    try {
        MediaSource* mediaSource = originalObj;
        String* type = toBrowserString(firstArg);
        SourceBuffer* buffer = mediaSource->addSourceBuffer(type);
        return buffer->scriptValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue removeSourceBufferFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    CHECK_TYPEOF(firstArg, SourceBuffer);

    try {
        MediaSource* mediaSource = (MediaSource*)originalObj;
        SourceBuffer* sourceBuffer = (SourceBuffer*)firstArg.asESPointer()
                                         ->asESObject()
                                         ->extraPointerData();
        mediaSource->removeSourceBuffer(sourceBuffer);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue endOfStreamFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, MediaSource);
    MediaSource* mediaSource =
        (MediaSource*)thisValue.asESPointer()->asESObject()->extraPointerData();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (!firstArg.isUndefinedOrNull() && !firstArg.isESString()) {
        THROW_ILLEGAL_INVOCATION();
    }
    String* error = String::emptyString;
    if (!firstArg.isUndefinedOrNull()) {
        error = toBrowserString(firstArg.toString());
    }
    try {
        mediaSource->endOfStream(error);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue isTypeSupportedFunction(ESVMInstance* instance)
{
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    String* type = toBrowserString(firstArg);
    bool res = MediaSource::isTypeSupported(type);
    if (res) {
        return ESValue(ESValue::ESTrueTag::ESTrue);
    } else {
        return ESValue(ESValue::ESFalseTag::ESFalse);
    }
}

static ESValue sourceBuffersFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    return ESValue(originalObj->sourceBuffers()->scriptValue());
}

static ESValue activeSourceBuffersFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    return ESValue(originalObj->activeSourceBuffers()->scriptValue());
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    MediaSource::ReadyState readyState = originalObj->readyState();

    if (readyState == MediaSource::Open) {
        return toJSString(
            originalObj->starFish()->staticStrings()->m_open.localName());
    } else if (readyState == MediaSource::Ended) {
        return toJSString(
            originalObj->starFish()->staticStrings()->m_ended.localName());
    }

    STARFISH_ASSERT(readyState == MediaSource::Closed);
    return toJSString(
        originalObj->starFish()->staticStrings()->m_closed.localName());
}

static ESValue durationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    return ESValue(originalObj->duration());
}

static ESValue durationSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double duration = firstArg.toNumber();
    if (std::isnan(duration)) {
        THROW_ILLEGAL_INVOCATION();
    }
    try {
        originalObj->setDuration(duration);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}
ESFunctionObject* bindingMediaSource(
    ScriptBindingInstance* scriptBindingInstance)
{
    // DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(MediaSource,
    // fetchData(scriptBindingInstance)->m_fnEventTarget);

    auto mediaSource = ESFunctionObject::create(NULL, mediaSourceFunction,
                                                ESString::create("MediaSource"),
                                                0, true, true);

    mediaSource->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    mediaSource->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    mediaSource->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->m_fnEventTarget->protoType());
    mediaSource->set__proto__(
        fetchData(scriptBindingInstance)->m_fnEventTarget);

    mediaSource->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("addSourceBuffer"), true, true, true,
        ESFunctionObject::create(NULL, addSourceBufferFunction,
                                 ESString::create("addSourceBuffer"), 1,
                                 false));

    mediaSource->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("removeSourceBuffer"), true, true, true,
        ESFunctionObject::create(NULL, removeSourceBufferFunction,
                                 ESString::create("removeSourceBuffer"), 1,
                                 false));

    mediaSource->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("endOfStream"), true, true, true,
        ESFunctionObject::create(NULL, endOfStreamFunction,
                                 ESString::create("endOfStream"), 1, false));

    // static bool isTypeSupported(type)
    mediaSource->defineDataProperty(
        ESString::create("isTypeSupported"), true, true, true,
        ESFunctionObject::create(NULL, isTypeSupportedFunction,
                                 ESString::create("isTypeSupported"), 1,
                                 false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        mediaSource->protoType().asESPointer()->asESObject(),
        ESString::create("sourceBuffers"), sourceBuffersFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        mediaSource->protoType().asESPointer()->asESObject(),
        ESString::create("activeSourceBuffers"), activeSourceBuffersFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        mediaSource->protoType().asESPointer()->asESObject(),
        ESString::create("readyState"), readyStateGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        mediaSource->protoType().asESPointer()->asESObject(),
        ESString::create("duration"), durationGetterFunction,
        durationSetterFunction);

    return mediaSource;
}
}
#endif
