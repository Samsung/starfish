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
    GENERATE_THIS_AND_CHECK_TYPE(NamedNodeMap);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

static ESValue itemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, NamedNodeMap);
    NamedNodeMap* self = (NamedNodeMap*)thisValue.asESPointer()
                             ->asESObject()
                             ->extraPointerData();
    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    TO_INDEX_UINT32(argValue, idx);
    if (idx != INVALID_INDEX && idx < self->length()) {
        Attr* elem = self->item(idx);
        if (elem != nullptr) {
            return elem->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}

static ESValue getNamedItemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, NamedNodeMap);

    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    if (argValue.isESString()) {
        NamedNodeMap* t = ((NamedNodeMap*)thisValue.asESPointer()
                               ->asESObject()
                               ->extraPointerData());
        String* key = toBrowserString(argValue);
        auto attrName = t->element()->document()->createAttributeName(key);
        Attr* elem = t->getNamedItem(attrName);
        if (elem != nullptr) {
            return elem->scriptValue();
        }
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue setNamedItemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, NamedNodeMap);
    NamedNodeMap* namedNodeMap = (NamedNodeMap*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData();
    STARFISH_ASSERT(namedNodeMap->element());

    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    CHECK_TYPEOF(argValue, Node);
    if (!((Node*)argValue.asESPointer()->asESObject()->extraPointerData())
             ->isAttr()) {
        THROW_ILLEGAL_INVOCATION()
    }
    Attr* passedAttr =
        (Attr*)argValue.asESPointer()->asESObject()->extraPointerData();
    Attr* toReturn = namedNodeMap->setNamedItem(passedAttr);
    if (toReturn) {
        return toReturn->scriptValue();
    } else {
        return ESValue(ESValue::ESNull);
    }
}

static ESValue removeNamedItemFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, NamedNodeMap);

    ESValue argValue = instance->currentExecutionContext()->readArgument(0);
    if (argValue.isESString()) {
        try {
            QualifiedName name(
                AtomicString::emptyAtomicString(),
                AtomicString::createAttrAtomicString(
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->starFish(),
                    argValue.asESString()->utf8Data()));
            Attr* old = ((NamedNodeMap*)thisValue.asESPointer()
                             ->asESObject()
                             ->extraPointerData())
                            ->getNamedItem(name);
            if (old == nullptr) {
                throw new DOMException(((NamedNodeMap*)thisValue.asESPointer()
                                            ->asESObject()
                                            ->extraPointerData())
                                           ->striptBindingInstance(),
                                       DOMException::Code::NOT_FOUND_ERR,
                                       nullptr);
            }
            Attr* toReturn = new Attr(old->document(),
                                      ((NamedNodeMap*)thisValue.asESPointer()
                                           ->asESObject()
                                           ->extraPointerData())
                                          ->striptBindingInstance(),
                                      name, old->value());
            ((NamedNodeMap*)thisValue.asESPointer()
                 ->asESObject()
                 ->extraPointerData())
                ->removeNamedItem(name);
            if (toReturn != nullptr) {
                return toReturn->scriptValue();
            }
        } catch (DOMException* e) {
            ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

ESFunctionObject* bindingNamedNodeMap(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NamedNodeMap,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    /* 4.8.1 Interface NamedNodeMap */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NamedNodeMapFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), false, false, false,
                             ESFunctionObject::create(NULL, itemFunction,
                                                      ESString::create("item"),
                                                      1, false));

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getNamedItem"), false, false, false,
            ESFunctionObject::create(NULL, getNamedItemFunction,
                                     ESString::create("getNamedItem"), 1,
                                     false));

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("setNamedItem"), false, false, false,
            ESFunctionObject::create(NULL, setNamedItemFunction,
                                     ESString::create("setNamedItem"), 1,
                                     false));

    NamedNodeMapFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("removeNamedItem"), false, false, false,
            ESFunctionObject::create(NULL, removeNamedItemFunction,
                                     ESString::create("removeNamedItem"), 1,
                                     false));

    return NamedNodeMapFunction;
}
}
