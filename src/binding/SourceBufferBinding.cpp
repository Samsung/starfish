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
#include "dom/TextTrackList.h"
#include "extra/SourceBuffer.h"
#include "extra/TimeRanges.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue modeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
    bool result;
    result = originalObj->updating();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue bufferedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare return value (empty when void)
    TimeRanges* result = nullptr;
    result = originalObj->buffered();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue timestampOffsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare return value (empty when void)
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
        THROW_EXCEPTION(FAILED_TO_SET_NONFINITE_PROPERTY_WHERE_EXPECTED_DOUBLE,
                        "timestampOffset", "SourceBuffer");
    }
    originalObj->setTimestampOffset(value0);
    return ESValue();
}

static ESValue textTracksGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare return value (empty when void)
    TextTrackList* result = nullptr;
    result = originalObj->textTracks();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue appendWindowStartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare return value (empty when void)
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
        THROW_EXCEPTION(FAILED_TO_SET_NONFINITE_PROPERTY_WHERE_EXPECTED_DOUBLE,
                        "appendWindowStart", "SourceBuffer");
    }
    originalObj->setAppendWindowStart(value0);
    return ESValue();
}

static ESValue appendWindowEndGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);
    // Declare return value (empty when void)
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

    return originalObj->onupdatestartEventListener();
}

static ESValue onupdatestartSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnupdatestartEventListener(arg0);

    return ESValue();
}

static ESValue onupdateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    return originalObj->onupdateEventListener();
}

static ESValue onupdateSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnupdateEventListener(arg0);

    return ESValue();
}

static ESValue onupdateendGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    return originalObj->onupdateendEventListener();
}

static ESValue onupdateendSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnupdateendEventListener(arg0);

    return ESValue();
}

static ESValue onerrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    return originalObj->onerrorEventListener();
}

static ESValue onerrorSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnerrorEventListener(arg0);

    return ESValue();
}

static ESValue onabortGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    return originalObj->onabortEventListener();
}

static ESValue onabortSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(SourceBuffer);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnabortEventListener(arg0);

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
        char buffer[1 + 1];
        snprintf(buffer, 1, "%zd", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "remove",
                        "SourceBuffer", "2", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();

    // Handle argument arg1
    double value1;
    value1 = arg1.toNumber();

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
    SourceBufferFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    SourceBufferFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    SourceBufferFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    SourceBufferFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());
    ESObject* SourceBufferPrototypeObj =
        SourceBufferFunction->protoType().asESPointer()->asESObject();

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

    return SourceBufferFunction;
}
}
#endif
