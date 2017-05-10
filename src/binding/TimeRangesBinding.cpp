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

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TimeRanges);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue startFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TimeRanges);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "start", "TimeRanges", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    double result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    // Call native function (nargs: 1)
    result = originalObj->start(value0);

    // Return ESValue from native value
    return ESValue(result);
}

static ESValue endFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TimeRanges);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "end", "TimeRanges", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    double result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    // Call native function (nargs: 1)
    result = originalObj->end(value0);

    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingTimeRanges(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TimeRangesString = ESString::create("TimeRanges");
    ESFunctionObject* TimeRangesFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, TimeRangesString, 0, true, true);
    ESObject* TimeRangesPrototypeObj =
        TimeRangesFunction->protoType().asESPointer()->asESObject();
    TimeRangesFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TimeRangesPrototypeObj->forceNonVectorHiddenClass(false);
    TimeRangesPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                             ->m_instance->globalObject()
                                             ->objectPrototype());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TimeRangesPrototypeObj, lengthString, lengthGetterFunction, nullptr);

    // Bind for functions
    ESString* startString = ESString::create("start");
    ESFunctionObject* startESFn =
        ESFunctionObject::create(nullptr, startFunction, startString, 1, false);
    TimeRangesPrototypeObj->defineDataProperty(startString, true, true, true,
                                               startESFn);

    ESString* endString = ESString::create("end");
    ESFunctionObject* endESFn =
        ESFunctionObject::create(nullptr, endFunction, endString, 1, false);
    TimeRangesPrototypeObj->defineDataProperty(endString, true, true, true,
                                               endESFn);

    return TimeRangesFunction;
}

void TimeRanges::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnTimeRanges()->protoType());

    postInit(instance);
}

bool TimeRanges::isTimeRanges() const
{
    return true;
}
}
#endif
