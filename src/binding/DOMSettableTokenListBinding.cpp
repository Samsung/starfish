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

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMSettableTokenList);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

static ESValue itemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMSettableTokenList);
    DOMTokenList* self = (DOMTokenList*)thisValue.asESPointer()
                             ->asESObject()
                             ->extraPointerData();
    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    TO_INDEX_UINT32(argValue, idx);
    if (idx != INVALID_INDEX && idx < self->length()) {
        Nullable<String*> elem = self->item(idx);
        if (elem.hasValue()) {
            return toJSString(elem.getValue());
        }
    }
    return ESValue(ESValue::ESNull);
}

static ESValue containsFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMSettableTokenList);
    try {
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            bool res = ((DOMTokenList*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData())
                           ->contains(toBrowserString(argValue.asESString()));
            return ESValue(res);
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue addFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMSettableTokenList);
    try {
        GCVector<String*> tokens;
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(i);
            if (argValue.isESString()) {
                String* aa = toBrowserString(argValue.asESString());
                tokens.push_back(aa);
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
        }
        if (argCount > 0) {
            ((DOMTokenList*)thisValue.asESPointer()->asESObject())
                ->add(&tokens);
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue removeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMSettableTokenList);
    try {
        GCVector<String*> tokens;
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(i);
            if (argValue.isESString()) {
                String* aa = toBrowserString(argValue.asESString());
                tokens.push_back(aa);
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
        }
        if (argCount > 0) {
            ((DOMTokenList*)thisValue.asESPointer()
                 ->asESObject()
                 ->extraPointerData())
                ->remove(&tokens);
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue toggleFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMSettableTokenList);
    try {
        int argCount = instance->currentExecutionContext()->argumentCount();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        ESValue forceValue;
        if (argCount >= 2) {
            forceValue = instance->currentExecutionContext()->readArgument(1);
        }
        if (argCount > 0 && argValue.isESString()) {
            bool didAdd;
            if (argCount == 1) {
                didAdd = ((DOMTokenList*)thisValue.asESPointer()
                              ->asESObject()
                              ->extraPointerData())
                             ->toggle(toBrowserString(argValue.asESString()),
                                      false, false);
            } else {
                ASSERT(forceValue.isBoolean());
                didAdd = ((DOMTokenList*)thisValue.asESPointer()
                              ->asESObject()
                              ->extraPointerData())
                             ->toggle(toBrowserString(argValue.asESString()),
                                      true, forceValue.asBoolean());
            }
            return ESValue(didAdd);
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue valueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMSettableTokenList);
    String* value = originalObj->value();
    return toJSString(value);
}

static ESValue valueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMSettableTokenList);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    originalObj->setValue(toBrowserString(v));
    return ESValue();
}

ESFunctionObject* bindingDOMSettableTokenList(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMSettableTokenList,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());
    /* 7.2 Interface DOMSettableTokenList */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMSettableTokenListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    DOMSettableTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), false, false, false,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    DOMSettableTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("contains"), false, false, false,
            ESFunctionObject::create(NULL, containsFunction,
                                     ESString::create("contains"), 1, false));

    DOMSettableTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("add"), false, false, false,
                             ESFunctionObject::create(NULL, addFunction,
                                                      ESString::create("add"),
                                                      1, false));

    DOMSettableTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), false, false, false,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 1, false));

    DOMSettableTokenListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("toggle"), false, false, false,
            ESFunctionObject::create(NULL, toggleFunction,
                                     ESString::create("toggle"), 1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMSettableTokenListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("value"), valueGetterFunction, valueSetterFunction);

    return DOMSettableTokenListFunction;
}
}
