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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue addEventListenerFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, EventTarget);
    if (instance->currentExecutionContext()->argumentCount() < 2) {
        auto msg = ESString::create(
            "Failed to execute 'addEventListener' on 'EventTaraget': "
            "needs 2 parameter.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
    ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
    if (firstArg.isESString() && secondArg.isESPointer() &&
        secondArg.asESPointer()->isESFunctionObject()) {
        ESString* argStr = firstArg.asESString();
        auto eventTypeName = String::fromUTF8(argStr->utf8Data());
        auto listener = new EventListener(secondArg);
        bool capture = thirdArg.isBoolean() ? thirdArg.toBoolean() : false;
        ((EventTarget*)thisValue.asESPointer()
             ->asESObject()
             ->extraPointerData())
            ->addEventListener(eventTypeName, listener, capture);
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&%s\n", eventTypeName->utf8Data());
#endif
    }
    return ESValue();
}

static ESValue removeEventListenerFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, EventTarget);
    if (instance->currentExecutionContext()->argumentCount() < 2) {
        auto msg = ESString::create(
            "Failed to execute 'removeEventListener' on "
            "'EventTaraget': needs 2 parameter.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
    ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
    if (firstArg.isESString() && secondArg.isESPointer() &&
        secondArg.asESPointer()->isESFunctionObject()) {
        // TODO: Verify valid event type. (e.g. click)
        ESString* argStr = firstArg.asESString();
        auto eventTypeName = String::fromUTF8(argStr->utf8Data());
        auto listener = new EventListener(secondArg);
        bool capture = thirdArg.isBoolean() ? thirdArg.toBoolean() : false;
        ((EventTarget*)thisValue.asESPointer()
             ->asESObject()
             ->extraPointerData())
            ->removeEventListener(eventTypeName, listener, capture);
    }
    return ESValue();
}

static ESValue dispatchEventListenerFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, EventTarget);
    int argCount = instance->currentExecutionContext()->argumentCount();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (firstArg.isUndefinedOrNull()) {
        ESString* msg = ESString::create(
            "Failed to execute 'dispatchEvent' on 'EventTarget': "
            "parameter 1 is not of type 'Event'.");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    bool ret = false;
    if (argCount == 1 && firstArg.isObject()) {
        if (!(firstArg.isObject() &&
              (firstArg.asESPointer()->asESObject()->extraData() ==
               kEscargotObjectCheckMagic) &&
              ((ScriptWrappable*)firstArg.asESPointer()
                   ->asESObject()
                   ->extraPointerData())
                  ->isEvent())) {
            auto msg = ESString::create(
                "Failed to execute 'dispatchEvent' on 'EventTarget': "
                "parameter 1 is not of type 'Event'.");
            instance->throwError(ESValue(TypeError::create(msg)));
        }
        Event* event =
            (Event*)firstArg.asESPointer()->asESObject()->extraPointerData();
        ret = ((EventTarget*)thisValue.asESPointer()
                   ->asESObject()
                   ->extraPointerData())
                  ->dispatchEvent(event);
    }
    return ESValue(ret);
}

ESFunctionObject* bindingEventTarget(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(EventTarget,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    auto fnAddEventListener = ESFunctionObject::create(
        NULL, addEventListenerFunction, ESString::create("addEventListener"), 0,
        false);
    fnAddEventListener->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("addEventListener"), true, true,
                             true, fnAddEventListener);

    auto fnRemoveEventListener = ESFunctionObject::create(
        NULL, removeEventListenerFunction,
        ESString::create("removeEventListener"), 0, false);
    fnRemoveEventListener->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("removeEventListener"), true,
                             true, true, fnRemoveEventListener);

    auto fnDispatchEvent =
        ESFunctionObject::create(NULL, dispatchEventListenerFunction,
                                 ESString::create("dispatchEvent"), 1, false);
    fnDispatchEvent->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("dispatchEvent"), true, true,
                             true, fnDispatchEvent);

    return EventTargetFunction;
}
}
