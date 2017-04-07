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

static ESValue appendBufferFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, SourceBuffer);
    SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

    try {
        if (false) {
        }
#ifdef USE_ES6_FEATURE
        else if (firstArg.isESPointer() &&
                 firstArg.asESPointer()->isESArrayBufferObject()) {
            ESArrayBufferObject* v =
                firstArg.asESPointer()->asESArrayBufferObject();
            sourceBuffer->appendBuffer((uint8_t*)v->data(), v->bytelength());
        } else if (firstArg.isESPointer() &&
                   firstArg.asESPointer()->isESArrayBufferView()) {
            ESArrayBufferView* v =
                firstArg.asESPointer()->asESArrayBufferView();
            uint8_t* p = (uint8_t*)v->buffer()->data();
            sourceBuffer->appendBuffer(p, v->bytelength());
        }
#endif
        else {
            ESVMInstance::currentInstance()->throwError(
                ESValue(TypeError::create(
                    ESString::create("Failed to execute 'appendBuffer' "
                                     "on 'SourceBuffer': No function "
                                     "was found that matched the "
                                     "signature provided."))));
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return ESValue(ESValue::ESUndefined);
}

static ESValue removeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, SourceBuffer);
    SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);

    try {
        sourceBuffer->remove(arg0.toNumber(), arg1.toNumber());
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return ESValue(ESValue::ESUndefined);
}

static ESValue abortFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, SourceBuffer);
    SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData();

    try {
        sourceBuffer->abort();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return ESValue(ESValue::ESUndefined);
}

static ESValue modeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    return ESValue(originalObj->mode());
}

static ESValue modeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double mode = firstArg.toNumber();
    if (std::isnan(mode)) {
        THROW_ILLEGAL_INVOCATION();
    }
    try {
        if (mode == SourceBuffer::AppendMode::Segments) {
            originalObj->setMode(SourceBuffer::AppendMode::Segments);
        } else if (mode == SourceBuffer::AppendMode::Sequence) {
            originalObj->setMode(SourceBuffer::AppendMode::Sequence);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue updatingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    return ESValue(originalObj->updating());
}

static ESValue bufferedGetterFunction(ESVMInstance* instance)
{
    try {
        GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
        TimeRanges* timeRanges = originalObj->buffered();
        if (timeRanges) {
            return timeRanges->scriptValue();
        }
        return ESValue(ESValue::ESUndefined);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue timestampOffsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    return ESValue(originalObj->timestampOffset());
}

static ESValue timestampOffsetSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double timeStampOffset = firstArg.toNumber();
    if (std::isnan(timeStampOffset)) {
        THROW_ILLEGAL_INVOCATION();
    }
    try {
        originalObj->setTimestampOffset(timeStampOffset);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue textTracksGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    TextTrackList* textTracks = originalObj->textTracks();
    if (textTracks) {
        return textTracks->scriptValue();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue appendWindowStartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    return ESValue(originalObj->appendWindowStart());
}

static ESValue appendWindowStartSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double timeStamp = firstArg.toNumber();
    if (std::isnan(timeStamp)) {
        THROW_ILLEGAL_INVOCATION();
    }
    try {
        originalObj->setAppendWindowStart(timeStamp);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue appendWindowEndGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    return ESValue(originalObj->appendWindowEnd());
}

static ESValue appendWindowEndSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    double timeStamp = firstArg.toNumber();
    if (std::isnan(timeStamp)) {
        THROW_ILLEGAL_INVOCATION();
    }
    try {
        originalObj->setAppendWindowEnd(timeStamp);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESUndefined);
}

#define DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC(eventName)                 \
    static ESValue on##eventName##GetterFunction(ESVMInstance* instance)  \
    {                                                                     \
        GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);                       \
        auto eventname =                                                  \
            (((Window*)instance->globalObject()->extraPointerData()))     \
                ->starFish()                                              \
                ->staticStrings()                                         \
                ->m_##eventName;                                          \
        return originalObj->attributeEventListener(eventname);            \
    }                                                                     \
                                                                          \
    static ESValue on##eventName##SetterFunction(ESVMInstance* instance)  \
    {                                                                     \
        GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);                       \
        auto eventname =                                                  \
            (((Window*)instance->globalObject()->extraPointerData()))     \
                ->starFish()                                              \
                ->staticStrings()                                         \
                ->m_##eventName;                                          \
        ESValue v = instance->currentExecutionContext()->readArgument(0); \
        if (v.isObject() ||                                               \
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) { \
            originalObj->setAttributeEventListener(eventname, v);         \
        } else {                                                          \
            originalObj->clearAttributeEventListener(eventname);          \
        }                                                                 \
        return ESValue();                                                 \
    }

DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC(updatestart);
DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC(update);
DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC(updateend);
DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC(error);
DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC(abort);
#undef DEFINE_SOURCEBUFFER_EVENT_HANDLER_FUNC

ESFunctionObject* bindingSourceBuffer(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        SourceBuffer, fetchData(scriptBindingInstance)->m_fnEventTarget);
    SourceBufferFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("appendBuffer"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, appendBufferFunction,
                                 ESString::create("appendBuffer"), 1, false));

    SourceBufferFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), true, true, true,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 2, false));

    SourceBufferFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("abort"), true, true, true,
                             ESFunctionObject::create(NULL, abortFunction,
                                                      ESString::create("abort"),
                                                      0, false));

    // TODO: appendStream

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("mode"), modeGetterFunction, modeSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("updating"), updatingGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("buffered"), bufferedGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("timestampOffset"), timestampOffsetGetterFunction,
        timestampOffsetSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("textTracks"), textTracksGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appendWindowStart"), appendWindowStartGetterFunction,
        appendWindowStartSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appendWindowEnd"), appendWindowEndGetterFunction,
        appendWindowEndSetterFunction);

// event handler

#define DEFINE_SOURCEBUFFER_EVENT_HANDLER(eventName)                      \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(              \
        SourceBufferFunction->protoType().asESPointer()->asESObject(),    \
        ESString::create("on" #eventName), on##eventName##GetterFunction, \
        on##eventName##SetterFunction);

    DEFINE_SOURCEBUFFER_EVENT_HANDLER(updatestart);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(update);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(updateend);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(error);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(abort);
#undef DEFINE_SOURCEBUFFER_EVENT_HANDLER

    return SourceBufferFunction;
}
}
#endif
