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

#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "extra/History.h"

namespace StarFish {

using namespace escargot;

static ESValue goFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);

    if (instance->currentExecutionContext()->argumentCount() > 0) {
        originalObj->go(
            instance->currentExecutionContext()->readArgument(0).asInt32());
    } else {
        originalObj->go(0);
    }

    return ESValue();
}

static ESValue backFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);
    originalObj->back();
    return ESValue();
}

static ESValue forwardFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);
    originalObj->forward();
    return ESValue();
}

static ESValue pushStateFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);

    if (instance->currentExecutionContext()->argumentCount() >= 2) {
        // TODO: State value must be stored to form of
        // StructuredClone
        // Therefore, implement StructuredClone() to convert
        // state value
        String* state =
            jsonStringify(instance->currentExecutionContext()->readArgument(0));
        String* title = String::fromUTF8(instance->currentExecutionContext()
                                             ->readArgument(1)
                                             .toString()
                                             ->utf8Data());
        String* url = String::fromUTF8(instance->currentExecutionContext()
                                           ->readArgument(2)
                                           .toString()
                                           ->utf8Data());
        originalObj->pushState(state, title, url);
    } else {
        auto msg = ESString::create(
            "Failed to execute 'pushState' on 'History': 2 "
            "arguments required, but only 0 present.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }
    return ESValue();
}

static ESValue replaceStateFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);

    if (instance->currentExecutionContext()->argumentCount() >= 2) {
        // TODO: State value must be stored to form of
        // StructuredClone
        // Therefore, implement StructuredClone() to convert
        // state value
        String* state =
            jsonStringify(instance->currentExecutionContext()->readArgument(0));
        String* title = String::fromUTF8(instance->currentExecutionContext()
                                             ->readArgument(1)
                                             .toString()
                                             ->utf8Data());
        String* url = String::fromUTF8(instance->currentExecutionContext()
                                           ->readArgument(2)
                                           .toString()
                                           ->utf8Data());
        originalObj->replaceState(state, title, url);
    } else {
        auto msg = ESString::create(
            "Failed to execute 'pushState' on 'History': 2 "
            "arguments required, but only 0 present.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }
    return ESValue();
}

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);
    return ESValue(originalObj->length());
}

static ESValue stateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HistoryObject, History);
    if (!originalObj->state()->equals(String::emptyString)) {
        return parseJSON(originalObj->state());
    }
    return ScriptValueNull;
}

ESFunctionObject* bindingHistory(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(History, fetchData(scriptBindingInstance)
                                                 ->m_instance->globalObject()
                                                 ->objectPrototype());
    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("go"), false, false, false,
                             ESFunctionObject::create(nullptr, goFunction,
                                                      ESString::create("go"), 1,
                                                      false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("back"), false, false, false,
                             ESFunctionObject::create(nullptr, backFunction,
                                                      ESString::create("back"),
                                                      1, false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("forward"), false, false, false,
            ESFunctionObject::create(nullptr, forwardFunction,
                                     ESString::create("forward"), 1, false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("pushState"), false, false, false,
            ESFunctionObject::create(nullptr, pushStateFunction,
                                     ESString::create("pushState"), 1, false));

    HistoryFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("replaceState"), false, false, false,
            ESFunctionObject::create(nullptr, replaceStateFunction,
                                     ESString::create("pushState"), 1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HistoryFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HistoryFunction->protoType().asESPointer()->asESObject(),
        ESString::create("state"), stateGetterFunction, nullptr);
    return HistoryFunction;
}
}
