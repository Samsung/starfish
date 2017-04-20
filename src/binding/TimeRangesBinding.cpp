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
#include "extra/TimeRanges.h"

namespace StarFish {

using namespace escargot;

static ESValue startFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TimeRanges);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    TO_INDEX_UINT32(firstArg, idx);
    if (idx != INVALID_INDEX && idx < originalObj->length()) {
        return ESValue(originalObj->start(idx));
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue endFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TimeRanges);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    TO_INDEX_UINT32(firstArg, idx);
    if (idx != INVALID_INDEX && idx < originalObj->length()) {
        return ESValue(originalObj->end(idx));
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TimeRanges);
    return ESValue(originalObj->length());
}

ESFunctionObject* bindingTimeRanges(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(TimeRanges, fetchData(scriptBindingInstance)
                                                    ->m_instance->globalObject()
                                                    ->objectPrototype());

    TimeRangesFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("start"), true, true, true,
                             ESFunctionObject::create(NULL, startFunction,
                                                      ESString::create("start"),
                                                      0, false));

    TimeRangesFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("end"), true, true, true,
                             ESFunctionObject::create(NULL, endFunction,
                                                      ESString::create("end"),
                                                      0, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TimeRangesFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    return TimeRangesFunction;
}
}
#endif
