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

static ESValue progressEventFunction(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);

    if (argCount == 0) {
        auto msg = ESString::create(
            "Failed to construct 'ProgressEvent': 1 argument required, "
            "but only 0 present.");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
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
            ESString* msg = ESString::create(
                "Failed to construct 'ProgressEvent': parameter 2 "
                "('eventInitDict') is not an object.");
            instance->throwError(ESValue(TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

static ESValue lengthComputableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    Event* e = originalObj;
    if (e->isProgressEvent()) {
        bool lengthComputable = e->asProgressEvent()->lengthComputable();
        return ESValue(lengthComputable);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue loadedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    Event* e = originalObj;
    if (e->isProgressEvent()) {
        unsigned long long loaded = e->asProgressEvent()->loaded();
        return ESValue(loaded);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue totalGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    Event* e = originalObj;
    if (e->isProgressEvent()) {
        unsigned long long total = e->asProgressEvent()->total();
        return ESValue(total);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

ESFunctionObject* bindingProgressEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    /* Progress Events */
    auto fnProgressEvent = ESFunctionObject::create(
        NULL, progressEventFunction, ESString::create("ProgressEvent"), 1, true,
        true);
    fnProgressEvent->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    fnProgressEvent->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnEvent()->protoType());
    fnProgressEvent->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    // fetchData(scriptBindingInstance)->m_instance->globalObject()
    //                                 ->defineDataProperty
    //           (ESString::create("ProgressEvent"), true, false,
    //           true, progressEventFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnProgressEvent->protoType().asESPointer()->asESObject(),
        ESString::create("lengthComputable"), lengthComputableGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnProgressEvent->protoType().asESPointer()->asESObject(),
        ESString::create("loaded"), loadedGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnProgressEvent->protoType().asESPointer()->asESObject(),
        ESString::create("total"), totalGetterFunction, nullptr);

    return fnProgressEvent;
}
}
