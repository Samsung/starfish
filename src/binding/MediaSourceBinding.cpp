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
#include "dom/DOMException.h"
#include "extra/SourceBufferList.h"
#include "extra/SourceBuffer.h"
#include "extra/MediaSource.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue mediasourceConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "MediaSource");
    }
    MediaSource* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 0)
    result = new MediaSource(callWith);
    return result->scriptValue();
}

// Implement for attributes
static ESValue sourceBuffersGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    // Declare native value (empty when type is void)
    SourceBufferList* result = nullptr;
    result = originalObj->sourceBuffers();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue activeSourceBuffersGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    // Declare native value (empty when type is void)
    SourceBufferList* result = nullptr;
    result = originalObj->activeSourceBuffers();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->readyState();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue durationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->duration();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue durationSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setDuration(value0);
    return ESValue();
}

// Implement for functions
static ESValue addSourceBufferFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "addSourceBuffer", "MediaSource", "1", buffer);
    }
    // Declare native value (empty when type is void)
    SourceBuffer* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->addSourceBuffer(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue removeSourceBufferFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "removeSourceBuffer", "MediaSource", "1", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    SourceBuffer* value0 = nullptr;
    CHECK_TYPEOF(arg0, SourceBuffer);
    value0 =
        (SourceBuffer*)(arg0.asESPointer()->asESObject()->extraPointerData());

    // Call native function (nargs: 1)
    try {
        originalObj->removeSourceBuffer(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue endOfStreamFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    size_t validArgCount = 1;
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (arg0.isUndefined()) {
        validArgCount--;
    } else {
        value0 = toBrowserString(arg0);
    }
    // Call native function (nargs: 0-1)
    try {
        if (validArgCount == 0) {
            originalObj->endOfStream();
        } else if (validArgCount == 1) {
            originalObj->endOfStream(value0);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue isTypeSupportedFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(MediaSource);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "isTypeSupported", "MediaSource", "1", buffer);
    }
    // Declare native value (empty when type is void)
    bool result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = MediaSource::isTypeSupported(value0);

    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingMediaSource(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* MediaSourceString = ESString::create("MediaSource");
    ESFunctionObject* MediaSourceFunction = ESFunctionObject::create(
        nullptr, mediasourceConstructor, MediaSourceString, 0, true, true);
    ESObject* MediaSourcePrototypeObj =
        MediaSourceFunction->protoType().asESPointer()->asESObject();
    MediaSourceFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    MediaSourcePrototypeObj->forceNonVectorHiddenClass(false);
    MediaSourcePrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    MediaSourceFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    // Bind for attributes
    ESString* sourceBuffersString = ESString::create("sourceBuffers");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourcePrototypeObj, sourceBuffersString,
        sourceBuffersGetterFunction, nullptr);

    ESString* activeSourceBuffersString =
        ESString::create("activeSourceBuffers");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourcePrototypeObj, activeSourceBuffersString,
        activeSourceBuffersGetterFunction, nullptr);

    ESString* readyStateString = ESString::create("readyState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourcePrototypeObj, readyStateString, readyStateGetterFunction,
        nullptr);

    ESString* durationString = ESString::create("duration");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourcePrototypeObj, durationString, durationGetterFunction,
        durationSetterFunction);

    // Bind for functions
    ESString* addSourceBufferString = ESString::create("addSourceBuffer");
    ESFunctionObject* addSourceBufferESFn = ESFunctionObject::create(
        nullptr, addSourceBufferFunction, addSourceBufferString, 1, false);
    MediaSourcePrototypeObj->defineDataProperty(
        addSourceBufferString, true, true, true, addSourceBufferESFn);

    ESString* removeSourceBufferString = ESString::create("removeSourceBuffer");
    ESFunctionObject* removeSourceBufferESFn =
        ESFunctionObject::create(nullptr, removeSourceBufferFunction,
                                 removeSourceBufferString, 1, false);
    MediaSourcePrototypeObj->defineDataProperty(
        removeSourceBufferString, true, true, true, removeSourceBufferESFn);

    ESString* endOfStreamString = ESString::create("endOfStream");
    ESFunctionObject* endOfStreamESFn = ESFunctionObject::create(
        nullptr, endOfStreamFunction, endOfStreamString, 0, false);
    MediaSourcePrototypeObj->defineDataProperty(endOfStreamString, true, true,
                                                true, endOfStreamESFn);

    ESString* isTypeSupportedString = ESString::create("isTypeSupported");
    ESFunctionObject* isTypeSupportedESFn = ESFunctionObject::create(
        nullptr, isTypeSupportedFunction, isTypeSupportedString, 1, false);
    MediaSourceFunction->defineDataProperty(isTypeSupportedString, true, true,
                                            true, isTypeSupportedESFn);

    return MediaSourceFunction;
}
}
#endif
