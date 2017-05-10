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
#include "extra/TimeRanges.h"
#include "dom/TextTrackList.h"
#include "extra/SourceBuffer.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue modeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->mode();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue modeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setMode(value0);
    return ESValue();
}

static ESValue updatingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->updating();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue bufferedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    TimeRanges* result = nullptr;
    result = originalObj->buffered();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue timestampOffsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->timestampOffset();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue timestampOffsetSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "timestampOffset",
                        "SourceBuffer");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
        THROW_EXCEPTION(msg);
    }
    originalObj->setTimestampOffset(value0);
    return ESValue();
}

static ESValue textTracksGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    TextTrackList* result = nullptr;
    result = originalObj->textTracks();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue appendWindowStartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->appendWindowStart();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue appendWindowStartSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "appendWindowStart",
                        "SourceBuffer");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
        THROW_EXCEPTION(msg);
    }
    originalObj->setAppendWindowStart(value0);
    return ESValue();
}

static ESValue appendWindowEndGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->appendWindowEnd();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue appendWindowEndSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setAppendWindowEnd(value0);
    return ESValue();
}

static ESValue onupdatestartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onupdatestart();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onupdatestartSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnupdatestart(value0);
    return ESValue();
}

static ESValue onupdateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onupdate();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onupdateSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnupdate(value0);
    return ESValue();
}

static ESValue onupdateendGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onupdateend();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onupdateendSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnupdateend(value0);
    return ESValue();
}

static ESValue onerrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onerror();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onerrorSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnerror(value0);
    return ESValue();
}

static ESValue onabortGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onabort();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onabortSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnabort(value0);
    return ESValue();
}

// Implement for functions
extern ESValue appendBufferSourceBufferFunction(ESVMInstance* instance);

static ESValue abortFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    try {
        originalObj->abort();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue removeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "remove", "SourceBuffer",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    double value1;
    value1 = arg1.toNumber();
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    if (!std::isfinite(value0)) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "remove",
                        "SourceBuffer");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, reason);
        THROW_EXCEPTION(msg);
    }
    // Call native function (nargs: 2)
    try {
        originalObj->remove(value0, value1);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingSourceBuffer(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* SourceBufferString = ESString::create("SourceBuffer");
    ESFunctionObject* SourceBufferFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, SourceBufferString, 0, true, true);
    ESObject* SourceBufferPrototypeObj =
        SourceBufferFunction->protoType().asESPointer()->asESObject();
    SourceBufferFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    SourceBufferPrototypeObj->forceNonVectorHiddenClass(false);
    SourceBufferPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    SourceBufferFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    // Bind for attributes
    ESString* modeString = ESString::create("mode");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, modeString, modeGetterFunction,
        modeSetterFunction);

    ESString* updatingString = ESString::create("updating");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, updatingString, updatingGetterFunction,
        nullptr);

    ESString* bufferedString = ESString::create("buffered");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, bufferedString, bufferedGetterFunction,
        nullptr);

    ESString* timestampOffsetString = ESString::create("timestampOffset");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, timestampOffsetString,
        timestampOffsetGetterFunction, timestampOffsetSetterFunction);

    ESString* textTracksString = ESString::create("textTracks");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, textTracksString, textTracksGetterFunction,
        nullptr);

    ESString* appendWindowStartString = ESString::create("appendWindowStart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, appendWindowStartString,
        appendWindowStartGetterFunction, appendWindowStartSetterFunction);

    ESString* appendWindowEndString = ESString::create("appendWindowEnd");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, appendWindowEndString,
        appendWindowEndGetterFunction, appendWindowEndSetterFunction);

    ESString* onupdatestartString = ESString::create("onupdatestart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, onupdatestartString,
        onupdatestartGetterFunction, onupdatestartSetterFunction);

    ESString* onupdateString = ESString::create("onupdate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, onupdateString, onupdateGetterFunction,
        onupdateSetterFunction);

    ESString* onupdateendString = ESString::create("onupdateend");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, onupdateendString, onupdateendGetterFunction,
        onupdateendSetterFunction);

    ESString* onerrorString = ESString::create("onerror");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, onerrorString, onerrorGetterFunction,
        onerrorSetterFunction);

    ESString* onabortString = ESString::create("onabort");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferPrototypeObj, onabortString, onabortGetterFunction,
        onabortSetterFunction);

    // Bind for functions
    ESString* appendBufferString = ESString::create("appendBuffer");
    ESFunctionObject* appendBufferSourceBufferESFn =
        ESFunctionObject::create(nullptr, appendBufferSourceBufferFunction,
                                 appendBufferString, 1, false);
    SourceBufferPrototypeObj->defineDataProperty(
        appendBufferString, true, true, true, appendBufferSourceBufferESFn);

    ESString* abortString = ESString::create("abort");
    ESFunctionObject* abortESFn =
        ESFunctionObject::create(nullptr, abortFunction, abortString, 0, false);
    SourceBufferPrototypeObj->defineDataProperty(abortString, true, true, true,
                                                 abortESFn);

    ESString* removeString = ESString::create("remove");
    ESFunctionObject* removeESFn = ESFunctionObject::create(
        nullptr, removeFunction, removeString, 2, false);
    SourceBufferPrototypeObj->defineDataProperty(removeString, true, true, true,
                                                 removeESFn);

    return SourceBufferFunction;
}

void SourceBuffer::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnSourceBuffer()->protoType());

    postInit(instance);
}

bool SourceBuffer::isSourceBuffer() const
{
    return true;
}
}
#endif
