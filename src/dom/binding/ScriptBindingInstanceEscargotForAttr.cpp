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

static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    String* n = ((Attr*)originalObj)->name().localName();
    return toJSString(n);
}

static ESValue valueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    String* value = ((Attr*)originalObj)->value();
    return toJSString(value);
}

static ESValue valueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    ((Attr*)originalObj)->setValue(toBrowserString(v));
    // FIXME(JMP): Actually this function have to return old Attr's value
    // but we have to modify 'typedef void (*ESNativeSetter)(...)' in
    // escargot/src/runtime/ESValue.h
    // Because this need to many changes, we do the modification latter
    return ESValue();
}

static ESValue ownerElementGetterFunction(ESObject* obj, ESObject* originalObj,
                                          ESString* propertyName)
{
    CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
    if (!((Node*)originalObj->extraPointerData())->isAttr()) {
        THROW_ILLEGAL_INVOCATION()
    }

    Element* elem = ((Attr*)originalObj->extraPointerData())->ownerElement();
    if (elem != nullptr) {
        return elem->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue specifiedGetterFunction(ESObject* obj, ESObject* originalObj,
                                       ESString* propertyName)
{
    CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
    if (!((Node*)originalObj->extraPointerData())->isAttr()) {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(true);
}

ESFunctionObject* bindingAttr(ScriptBindingInstance* scriptBindingInstance)
{
    /* 4.8.2 Interface Attr */
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Attr, fetchData(scriptBindingInstance)->node());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(),
        ESString::create("name"), nameGetterFunction, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(),
        ESString::create("localName"), nameGetterFunction, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(),
        ESString::create("value"), valueGetterFunction, valueSetterFunction);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nodeValue"), valueGetterFunction,
        valueSetterFunction);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(),
        ESString::create("textContent"), valueGetterFunction,
        valueSetterFunction);

    AttrFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(ESString::create("ownerElement"),
                                 ownerElementGetterFunction, NULL, false, false,
                                 false);

    AttrFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(ESString::create("specified"),
                                 specifiedGetterFunction, NULL, false, false,
                                 false);

    return AttrFunction;
}
}
