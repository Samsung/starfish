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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/Attr.h"
#include "dom/DOMException.h"
#include "dom/Element.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue localNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    String* v = originalObj->localName();
    return toJSString(v);
}

static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    String* v = originalObj->name();
    return toJSString(v);
}

static ESValue valueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    String* v = originalObj->value();
    return toJSString(v);
}

static ESValue valueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    ESValue originalV = instance->currentExecutionContext()->readArgument(0);
    String* v = toBrowserString(originalV);
    originalObj->setValue(v);
    return ESValue();
}

static ESValue ownerElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    Element* v = originalObj->ownerElement();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue specifiedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    bool v = originalObj->specified();
    return ESValue(v);
}

ESFunctionObject* bindingAttr(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* AttrString = ESString::create("Attr");
    ESFunctionObject* AttrFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, AttrString, 1, true, true);

    AttrFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);

    AttrFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);

    AttrFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());

    AttrFunction->set__proto__(fetchData(scriptBindingInstance)->fnNode());

    // Bind for attributes
    ESString* localNameString = ESString::create("localName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(), localNameString,
        localNameGetterFunction, nullptr);

    ESString* nameString = ESString::create("name");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(), nameString,
        nameGetterFunction, nullptr);

    ESString* valueString = ESString::create("value");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(), valueString,
        valueGetterFunction, valueSetterFunction);

    ESString* ownerElementString = ESString::create("ownerElement");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(),
        ownerElementString, ownerElementGetterFunction, nullptr);

    ESString* specifiedString = ESString::create("specified");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrFunction->protoType().asESPointer()->asESObject(), specifiedString,
        specifiedGetterFunction, nullptr);

    return AttrFunction;
}
}
