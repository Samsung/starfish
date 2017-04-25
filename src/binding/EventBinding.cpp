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
#include "dom/Event.h"
#include "dom/EventTarget.h"

namespace StarFish {

using namespace escargot;

extern EventInit toEventInitFromESValue(ESVMInstance* instance, ESValue& from);
extern ESValue toESValueFromEventInit(ESVMInstance* instance, EventInit& from);

static ESValue eventConstructor(ESVMInstance* instance)
{
    // TODO Following TC need to be fixed to enable this code
    // test/reftest/web-platform-tests/dom/events/Event-constructors.html
    // if (!instance->currentExecutionContext()->isNewExpression()) {
    //     THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "Event");
    // }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[1 + 1];
        snprintf(buffer, 1, "%zd", argCount);
        THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARGS_NOT_ENOUGH, "Event",
                        "1", buffer);
    }
    size_t validArgCount = 2;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    EventInit value1;
    if (arg1.isUndefinedOrNull()) {
        validArgCount--;
    } else {
        value1 = toEventInitFromESValue(instance, arg1);
    }
    Event* result = nullptr;
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = new Event(value0);
    } else if (validArgCount == 2) {
        result = new Event(value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    String* v = originalObj->type();
    return toJSString(v);
}

static ESValue targetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    EventTarget* v = originalObj->target();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue currentTargetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    EventTarget* v = originalObj->currentTarget();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue eventPhaseGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    uint32_t v = originalObj->eventPhase();
    return ESValue(v);
}

static ESValue bubblesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    bool v = originalObj->bubbles();
    return ESValue(v);
}

static ESValue cancelableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    bool v = originalObj->cancelable();
    return ESValue(v);
}

static ESValue defaultPreventedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    bool v = originalObj->defaultPrevented();
    return ESValue(v);
}

static ESValue timeStampGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    DOMTimeStamp v = originalObj->timeStamp();
    return ESValue(v);
}

// Implement for functions
static ESValue stopPropagationFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->stopPropagation();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue stopImmediatePropagationFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->stopImmediatePropagation();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue preventDefaultFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare return value (empty when void)
    // Call native function (nargs: 0)
    originalObj->preventDefault();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* 3.2 Interface Event */
    auto fnEvent = ESFunctionObject::create(
        NULL, eventConstructor, ESString::create("Event"), 1, true, true);
    fnEvent->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(
        false);
    fnEvent->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    fnEvent->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    // fetchData(scriptBindingInstance)->m_instance->globalObject()
    //                                 ->defineDataProperty
    //           (ESString::create("Event"), true, false,
    //           true, eventFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("type"), typeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("target"), targetGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("currentTarget"), currentTargetGetterFunction,
        nullptr);

    fnEvent->asESObject()->defineDataProperty(
        ESString::create("NONE"), false, true, false, ESValue(Event::NONE));
    fnEvent->asESObject()->defineDataProperty(
        ESString::create("CAPTURING_PHASE"), false, true, false,
        ESValue(Event::CAPTURING_PHASE));
    fnEvent->asESObject()->defineDataProperty(ESString::create("AT_TARGET"),
                                              false, true, false,
                                              ESValue(Event::AT_TARGET));
    fnEvent->asESObject()->defineDataProperty(
        ESString::create("BUBBLING_PHASE"), false, true, false,
        ESValue(Event::BUBBLING_PHASE));

    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("NONE"), false, true, false, ESValue(Event::NONE));
    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("CAPTURING_PHASE"), false, true, false,
        ESValue(Event::CAPTURING_PHASE));
    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("AT_TARGET"), false, true, false,
        ESValue(Event::AT_TARGET));
    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("BUBBLING_PHASE"), false, true, false,
        ESValue(Event::BUBBLING_PHASE));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("eventPhase"), eventPhaseGetterFunction, nullptr);

    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("stopPropagation"), false, false, false,
        ESFunctionObject::create(NULL, stopPropagationFunction,
                                 ESString::create("stopPropagation"), 0,
                                 false));

    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("stopImmediatePropagation"), false, false, false,
        ESFunctionObject::create(NULL, stopImmediatePropagationFunction,
                                 ESString::create("stopImmediatePropagation"),
                                 0, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("bubbles"), bubblesGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("cancelable"), cancelableGetterFunction, nullptr);

    fnEvent->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("preventDefault"), false, false, false,
        ESFunctionObject::create(NULL, preventDefaultFunction,
                                 ESString::create("preventDefault"), 0, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("defaultPrevented"), defaultPreventedGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnEvent->protoType().asESPointer()->asESObject(),
        ESString::create("timeStamp"), timeStampGetterFunction, nullptr);

    return fnEvent;
}
}
