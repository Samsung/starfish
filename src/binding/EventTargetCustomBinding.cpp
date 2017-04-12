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

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

ESValue addEventListenerEventTargetFunction(ESVMInstance* instance)
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

ESValue removeEventListenerEventTargetFunction(ESVMInstance* instance)
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
}
