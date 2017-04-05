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

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMTokenList);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

static ESValue itemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);

    DOMTokenList* self = (DOMTokenList*)(thisValue.asESPointer()
                                             ->asESObject()
                                             ->extraPointerData());
    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    TO_INDEX_UINT32(argValue, idx);
    if (idx != INVALID_INDEX && idx < self->length()) {
        String* elem = self->item(idx);
        return toJSString(elem);
    }
    return ESValue(ESValue::ESNull);
}

static ESValue containsFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);
    try {
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        ESString* argStr = argValue.toString();
        bool res = ((DOMTokenList*)thisValue.asESPointer()
                        ->asESObject()
                        ->extraPointerData())
                       ->contains(toBrowserString(argStr));
        return ESValue(res);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue addFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);
    try {
        GCVector<String*> tokens;
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(i);
            ESString* argStr = argValue.toString();
            String* aa = toBrowserString(argStr);
            tokens.push_back(aa);
        }
        if (argCount > 0) {
            ((DOMTokenList*)thisValue.asESPointer()
                 ->asESObject()
                 ->extraPointerData())
                ->add(&tokens);
        }
        return ESValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue removeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);
    try {
        GCVector<String*> tokens;
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(i);
            ESString* argStr = argValue.toString();
            String* aa = toBrowserString(argStr);
            tokens.push_back(aa);
        }
        if (argCount > 0) {
            ((DOMTokenList*)thisValue.asESPointer()
                 ->asESObject()
                 ->extraPointerData())
                ->remove(&tokens);
        }
        return ESValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue toggleFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);
    try {
        int argCount = instance->currentExecutionContext()->argumentCount();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        ESValue forceValue;

        if (argCount == 0) {
            instance->throwError(ESValue(
                TypeError::create(ESString::create("Not enough arguments"))));
        }
        if (argCount >= 2) {
            forceValue = instance->currentExecutionContext()->readArgument(1);
        }
        if (argCount > 0) {
            ESString* argStr = argValue.toString();
            bool didAdd;
            if (argCount == 1) {
                didAdd = ((DOMTokenList*)thisValue.asESPointer()
                              ->asESObject()
                              ->extraPointerData())
                             ->toggle(toBrowserString(argStr), false, false);
            } else {
                if (forceValue.isUndefined()) {
                    didAdd =
                        ((DOMTokenList*)thisValue.asESPointer()
                             ->asESObject()
                             ->extraPointerData())
                            ->toggle(toBrowserString(argStr), false, false);
                } else {
                    ASSERT(forceValue.isBoolean());
                    didAdd = ((DOMTokenList*)thisValue.asESPointer()
                                  ->asESObject()
                                  ->extraPointerData())
                                 ->toggle(toBrowserString(argStr), true,
                                          forceValue.asBoolean());
                }
            }
            return ESValue(didAdd);
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue toStringFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);
    String* str = ((DOMTokenList*)thisValue.asESPointer()
                       ->asESObject()
                       ->extraPointerData())
                      ->toString();
    return toJSString(str);
}

// https://dom.spec.whatwg.org/#interface-domtokenlist
ESFunctionObject* bindingDOMTokenList(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMTokenList,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    /* 7.1 Interface DOMTokenList */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMTokenListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), false, false, false,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("contains"), false, false, false,
            ESFunctionObject::create(NULL, containsFunction,
                                     ESString::create("contains"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("add"), false, false, false,
                             ESFunctionObject::create(NULL, addFunction,
                                                      ESString::create("add"),
                                                      1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), false, false, false,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("toggle"), false, false, false,
            ESFunctionObject::create(NULL, toggleFunction,
                                     ESString::create("toggle"), 1, false));

    DOMTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("toString"), true, false, true,
            ESFunctionObject::create(NULL, toStringFunction,
                                     ESString::create("toString"), 1, false));

    return DOMTokenListFunction;
}
}
