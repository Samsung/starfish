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

#include "dom/EventTarget.h"
#include "dom/Event.h"

namespace StarFish {

using namespace escargot;

extern EventInit toEventInitFromESValue(ESVMInstance* instance, ESValue& from);
extern ESValue toESValueFromEventInit(ESVMInstance* instance, EventInit& from);

// Implement for constructor
static ESValue eventConstructor(ESVMInstance* instance)
{
    // TODO Following TC need to be fixed to enable this code
    // test/reftest/web-platform-tests/dom/events/Event-constructors.html
    // if (!instance->currentExecutionContext()->isNewExpression()) {
    //     THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "Event");
    // }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
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
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->type();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue targetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    EventTarget* result = nullptr;
    result = originalObj->target();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue currentTargetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    EventTarget* result = nullptr;
    result = originalObj->currentTarget();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue eventPhaseGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->eventPhase();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue bubblesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->bubbles();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue cancelableGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->cancelable();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue defaultPreventedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->defaultPrevented();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue timeStampGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    uint64_t result;
    result = originalObj->timeStamp();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue stopPropagationFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->stopPropagation();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue stopImmediatePropagationFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->stopImmediatePropagation();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue preventDefaultFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->preventDefault();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingEvent(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* EventString = ESString::create("Event");
    ESFunctionObject* EventFunction = ESFunctionObject::create(
        nullptr, eventConstructor, EventString, 1, true, true);
    ESObject* EventPrototypeObj =
        EventFunction->protoType().asESPointer()->asESObject();
    EventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    EventPrototypeObj->forceNonVectorHiddenClass(false);
    EventPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    // Bind for constants
    ESString* NONEString = ESString::create("NONE");
    ESValue NONEValue = ESValue(0);
    EventPrototypeObj->defineDataProperty(NONEString, false, true, false,
                                          NONEValue);

    EventFunction->defineDataProperty(NONEString, false, true, false,
                                      NONEValue);

    ESString* CAPTURING_PHASEString = ESString::create("CAPTURING_PHASE");
    ESValue CAPTURING_PHASEValue = ESValue(1);
    EventPrototypeObj->defineDataProperty(CAPTURING_PHASEString, false, true,
                                          false, CAPTURING_PHASEValue);

    EventFunction->defineDataProperty(CAPTURING_PHASEString, false, true, false,
                                      CAPTURING_PHASEValue);

    ESString* AT_TARGETString = ESString::create("AT_TARGET");
    ESValue AT_TARGETValue = ESValue(2);
    EventPrototypeObj->defineDataProperty(AT_TARGETString, false, true, false,
                                          AT_TARGETValue);

    EventFunction->defineDataProperty(AT_TARGETString, false, true, false,
                                      AT_TARGETValue);

    ESString* BUBBLING_PHASEString = ESString::create("BUBBLING_PHASE");
    ESValue BUBBLING_PHASEValue = ESValue(3);
    EventPrototypeObj->defineDataProperty(BUBBLING_PHASEString, false, true,
                                          false, BUBBLING_PHASEValue);

    EventFunction->defineDataProperty(BUBBLING_PHASEString, false, true, false,
                                      BUBBLING_PHASEValue);

    // Bind for attributes
    ESString* typeString = ESString::create("type");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, typeString, typeGetterFunction, nullptr);

    ESString* targetString = ESString::create("target");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, targetString, targetGetterFunction, nullptr);

    ESString* currentTargetString = ESString::create("currentTarget");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, currentTargetString, currentTargetGetterFunction,
        nullptr);

    ESString* eventPhaseString = ESString::create("eventPhase");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, eventPhaseString, eventPhaseGetterFunction, nullptr);

    ESString* bubblesString = ESString::create("bubbles");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, bubblesString, bubblesGetterFunction, nullptr);

    ESString* cancelableString = ESString::create("cancelable");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, cancelableString, cancelableGetterFunction, nullptr);

    ESString* defaultPreventedString = ESString::create("defaultPrevented");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, defaultPreventedString,
        defaultPreventedGetterFunction, nullptr);

    ESString* timeStampString = ESString::create("timeStamp");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        EventPrototypeObj, timeStampString, timeStampGetterFunction, nullptr);

    // Bind for functions
    ESString* stopPropagationString = ESString::create("stopPropagation");
    ESFunctionObject* stopPropagationESFn = ESFunctionObject::create(
        nullptr, stopPropagationFunction, stopPropagationString, 0, false);
    EventPrototypeObj->defineDataProperty(stopPropagationString, true, true,
                                          true, stopPropagationESFn);

    ESString* stopImmediatePropagationString =
        ESString::create("stopImmediatePropagation");
    ESFunctionObject* stopImmediatePropagationESFn =
        ESFunctionObject::create(nullptr, stopImmediatePropagationFunction,
                                 stopImmediatePropagationString, 0, false);
    EventPrototypeObj->defineDataProperty(stopImmediatePropagationString, true,
                                          true, true,
                                          stopImmediatePropagationESFn);

    ESString* preventDefaultString = ESString::create("preventDefault");
    ESFunctionObject* preventDefaultESFn = ESFunctionObject::create(
        nullptr, preventDefaultFunction, preventDefaultString, 0, false);
    EventPrototypeObj->defineDataProperty(preventDefaultString, true, true,
                                          true, preventDefaultESFn);

    return EventFunction;
}
}
