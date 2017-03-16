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

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* 3.2 Interface Event */
    auto eventFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            int argCount = instance->currentExecutionContext()->argumentCount();
            ESValue firstArg =
                instance->currentExecutionContext()->readArgument(0);
            ESValue secondArg =
                instance->currentExecutionContext()->readArgument(1);

            if (argCount == 0) {
                auto msg = ESString::create(
                    "Failed to construct 'Event': 1 argument required, but "
                    "only 0 present.");
                instance->throwError(ESValue(TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else if (argCount == 1) {
                ESString* type = firstArg.toString();
                auto event = new Event(String::fromUTF8(type->utf8Data()));
                return event->scriptValue();
            } else {
                if (secondArg.isObject() || secondArg.isUndefinedOrNull()) {
                    ESString* type = firstArg.toString();
                    bool canBubbles = false;
                    bool canCancelable = false;
                    if (!secondArg.isUndefinedOrNull()) {
                        ESValue bubbles =
                            secondArg.asESPointer()->asESObject()->get(
                                ESString::create("bubbles"));
                        ESValue cancelable =
                            secondArg.asESPointer()->asESObject()->get(
                                ESString::create("cancelable"));
                        canBubbles = bubbles.isBoolean() ? bubbles.asBoolean()
                                                         : canBubbles;
                        canCancelable = cancelable.isBoolean()
                                            ? cancelable.asBoolean()
                                            : canCancelable;
                    }
#ifdef STARFISH_TC_COVERAGE
                    if (canBubbles) {
                        STARFISH_LOG_INFO("&&&EventInit::bubbles\n");
                    }
                    if (canCancelable) {
                        STARFISH_LOG_INFO("&&&EventInit::cancelable\n");
                    }
#endif
                    auto event =
                        new Event(String::fromUTF8(type->utf8Data()),
                                  EventInit(canBubbles, canCancelable));
                    return event->scriptValue();
                } else {
                    ESString* msg = ESString::create(
                        "Failed to construct 'Event': parameter 2 "
                        "('eventInitDict') is not an object.");
                    instance->throwError(ESValue(TypeError::create(msg)));
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }
        },
        ESString::create("Event"), 1, true, true);
    eventFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    eventFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    eventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    // fetchData(scriptBindingInstance)->m_instance->globalObject()
    //                                 ->defineDataProperty
    //           (ESString::create("Event"), true, false,
    //           true, eventFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("type"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            String* type = const_cast<String*>(originalObj->eventType());
            return toJSString(type);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("target"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            EventTarget* target = originalObj->target();
            if (target) {
                return target->scriptValue();
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("currentTarget"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            EventTarget* currentTarget = originalObj->currentTarget();
            if (currentTarget) {
                return currentTarget->scriptValue();
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);

    eventFunction->asESObject()->defineDataProperty(
        ESString::create("NONE"), false, true, false, ESValue(Event::NONE));
    eventFunction->asESObject()->defineDataProperty(
        ESString::create("CAPTURING_PHASE"), false, true, false,
        ESValue(Event::CAPTURING_PHASE));
    eventFunction->asESObject()->defineDataProperty(
        ESString::create("AT_TARGET"), false, true, false,
        ESValue(Event::AT_TARGET));
    eventFunction->asESObject()->defineDataProperty(
        ESString::create("BUBBLING_PHASE"), false, true, false,
        ESValue(Event::BUBBLING_PHASE));

    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("NONE"), false, true, false, ESValue(Event::NONE));
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("CAPTURING_PHASE"), false, true, false,
        ESValue(Event::CAPTURING_PHASE));
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("AT_TARGET"), false, true, false,
        ESValue(Event::AT_TARGET));
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("BUBBLING_PHASE"), false, true, false,
        ESValue(Event::BUBBLING_PHASE));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("eventPhase"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            unsigned short eventPhase = originalObj->eventPhase();
            return ESValue(eventPhase);
        },
        nullptr);

    auto stopPropagationFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventObject);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount == 0) {
                ((Event*)thisValue.asESPointer()
                     ->asESObject()
                     ->extraPointerData())
                    ->setStopPropagation();
            }
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("stopPropagation"), 0, false);
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("stopPropagation"), false, false, false,
        stopPropagationFunction);

    auto stopImmediatePropagationFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventObject);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount == 0) {
                ((Event*)thisValue.asESPointer()
                     ->asESObject()
                     ->extraPointerData())
                    ->setStopImmediatePropagation();
            }
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("stopImmediatePropagation"), 0, false);
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("stopImmediatePropagation"), false, false, false,
        stopImmediatePropagationFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("bubbles"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            bool bubbles = originalObj->bubbles();
            return ESValue(bubbles);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("cancelable"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            bool cancelable = originalObj->cancelable();
            return ESValue(cancelable);
        },
        nullptr);

    auto preventDefaultFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventObject);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount == 0) {
                ((Event*)thisValue.asESPointer()
                     ->asESObject()
                     ->extraPointerData())
                    ->preventDefault();
            }
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("preventDefault"), 0, false);
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("preventDefault"), false, false, false,
        preventDefaultFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("defaultPrevented"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            bool defaultPrevented = originalObj->defaultPrevented();
            return ESValue(defaultPrevented);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("timeStamp"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            DOMTimeStamp timeStamp = originalObj->timeStamp();
            return ESValue(timeStamp);
        },
        nullptr);

    return eventFunction;
}
}
