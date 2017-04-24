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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/ProgressEvent.h"

namespace StarFish {

using namespace escargot;

static ESValue progresseventConstructor(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);

    if (argCount == 0) {
        THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARGS_NOT_ENOUGH,
                        "ProgressEvent", "1", "0");
    } else if (argCount == 1) {
        ESString* type = firstArg.toString();
        auto event = new ProgressEvent(String::fromUTF8(type->utf8Data()));
        return event->scriptValue();
    } else {
        if (secondArg.isObject()) {
            ESString* type = firstArg.toString();
            ESObject* obj = secondArg.asESPointer()->asESObject();
            ESValue bubbles = obj->get(ESString::create("bubbles"));
            ESValue cancelable = obj->get(ESString::create("cancelable"));
            ESValue lengthComputable =
                obj->get(ESString::create("lengthComputable"));
            ESValue loaded = obj->get(ESString::create("loaded"));
            ESValue total = obj->get(ESString::create("total"));
            bool canBubbles = bubbles.isBoolean() ? bubbles.asBoolean() : false;
            bool canCancelable =
                cancelable.isBoolean() ? cancelable.asBoolean() : false;
            bool canLengthComputable = false;
            if (lengthComputable.isBoolean()) {
                canLengthComputable = lengthComputable.asBoolean();
            } else if (lengthComputable.isESString()) {
                canLengthComputable =
                    lengthComputable.asESString()->length() != 0 ? true : false;
            }
            unsigned long long loadedValue = 0;
            if (loaded.isNumber()) {
                loadedValue = loaded.asNumber();
            } else if (loaded.isESString()) {
                loadedValue = loaded.toUint32();
            }
            unsigned long long totalValue = 0;
            if (total.isNumber()) {
                totalValue = total.asNumber();
            } else if (loaded.isESString()) {
                totalValue = total.toUint32();
            }
#ifdef STARFISH_TC_COVERAGE
            if (type->isESString()) {
                STARFISH_LOG_INFO("ProgressEventInit&&&type\n");
            }
            if (total.isESString()) {
                STARFISH_LOG_INFO("ProgressEventInit&&&total\n");
            }
            if (canLengthComputable) {
                STARFISH_LOG_INFO("ProgressEventInit&&&lengthComputable\n");
            }
            if (loadedValue != 0) {
                STARFISH_LOG_INFO("ProgressEventInit&&&loaded\n");
            }
#endif
            auto event =
                new ProgressEvent(String::fromUTF8(type->utf8Data()),
                                  ProgressEventInit(canBubbles, canCancelable,
                                                    canLengthComputable,
                                                    loadedValue, totalValue));
            return event->scriptValue();
        } else {
            THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARG_TYPE_MISMATCH,
                            "ProgressEvent", "2", "eventInitDict", "object");
        }
    }
}

// Implement for attributes
static ESValue lengthComputableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ProgressEvent);
    // Declare return value (empty when void)
    bool result;
    result = originalObj->lengthComputable();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue loadedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ProgressEvent);
    // Declare return value (empty when void)
    double result;
    result = originalObj->loaded();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue totalGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ProgressEvent);
    // Declare return value (empty when void)
    double result;
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
    ProgressEventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    ProgressEventFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    ProgressEventFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            fetchData(scriptBindingInstance)->fnEvent()->protoType());
    ProgressEventFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEvent());

    // Bind for attributes
    ESString* lengthComputableString = ESString::create("lengthComputable");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ProgressEventFunction->protoType().asESPointer()->asESObject(),
        lengthComputableString, lengthComputableGetterFunction, nullptr);

    ESString* loadedString = ESString::create("loaded");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ProgressEventFunction->protoType().asESPointer()->asESObject(),
        loadedString, loadedGetterFunction, nullptr);

    ESString* totalString = ESString::create("total");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ProgressEventFunction->protoType().asESPointer()->asESObject(),
        totalString, totalGetterFunction, nullptr);

    return ProgressEventFunction;
}
}
