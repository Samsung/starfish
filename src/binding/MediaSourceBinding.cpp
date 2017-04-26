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
#include "extra/MediaSource.h"
#include "extra/SourceBuffer.h"
#include "extra/SourceBufferList.h"

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
    String* result = originalObj->readyStateAttr();

    return toJSString(result);
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
    if (arg0.isUndefinedOrNull()) {
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
    // DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(MediaSource,
    // fetchData(scriptBindingInstance)->m_fnEventTarget);

    auto mediaSource = ESFunctionObject::create(NULL, mediasourceConstructor,
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
