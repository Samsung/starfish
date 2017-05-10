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

#include "dom/ProgressEvent.h"

namespace StarFish {

using namespace escargot;

extern ProgressEventInit toProgressEventInitFromESValue(ESVMInstance* instance,
                                                        ESValue& from);
extern ESValue toESValueFromProgressEventInit(ESVMInstance* instance,
                                              ProgressEventInit& from);

// Implement for constructor
static ESValue progresseventConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "ProgressEvent");
        THROW_EXCEPTION(msg);
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "parseFromString",
                        "ProgressEvent", reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 2;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    ProgressEventInit value1;
    if (arg1.isUndefined()) {
        validArgCount--;
    } else {
        value1 = toProgressEventInitFromESValue(instance, arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    ProgressEvent* result = nullptr;
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = new ProgressEvent(value0);
    } else if (validArgCount == 2) {
        result = new ProgressEvent(value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
static ESValue lengthComputableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ProgressEvent);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->lengthComputable();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue loadedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ProgressEvent);
    // Declare native value (empty when type is void)
    uint64_t result;
    result = originalObj->loaded();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue totalGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ProgressEvent);
    // Declare native value (empty when type is void)
    uint64_t result;
    result = originalObj->total();
    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingProgressEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* ProgressEventString = ESString::create("ProgressEvent");
    ESFunctionObject* ProgressEventFunction = ESFunctionObject::create(
        nullptr, progresseventConstructor, ProgressEventString, 1, true, true);
    ESObject* ProgressEventPrototypeObj =
        ProgressEventFunction->protoType().asESPointer()->asESObject();
    ProgressEventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    ProgressEventPrototypeObj->forceNonVectorHiddenClass(false);
    ProgressEventPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEvent()->protoType());
    ProgressEventFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEvent());

    // Bind for attributes
    ESString* lengthComputableString = ESString::create("lengthComputable");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ProgressEventPrototypeObj, lengthComputableString,
        lengthComputableGetterFunction, nullptr);

    ESString* loadedString = ESString::create("loaded");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ProgressEventPrototypeObj, loadedString, loadedGetterFunction, nullptr);

    ESString* totalString = ESString::create("total");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ProgressEventPrototypeObj, totalString, totalGetterFunction, nullptr);

    return ProgressEventFunction;
}

void ProgressEvent::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnProgressEvent()->protoType());

    postInit(instance);
}

bool ProgressEvent::isProgressEvent() const
{
    return true;
}
}
