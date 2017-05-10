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

#include "dom/Element.h"
#include "dom/Attr.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue localNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->localName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->name();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue valueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->value();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue valueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setValue(value0);
    return ESValue();
}

static ESValue ownerElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->ownerElement();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue specifiedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Attr);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->specified();
    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingAttr(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* AttrString = ESString::create("Attr");
    ESFunctionObject* AttrFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, AttrString, 0, true, true);
    ESObject* AttrPrototypeObj =
        AttrFunction->protoType().asESPointer()->asESObject();
    AttrFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    AttrPrototypeObj->forceNonVectorHiddenClass(false);
    AttrPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());
    AttrFunction->set__proto__(fetchData(scriptBindingInstance)->fnNode());

    // Bind for attributes
    ESString* localNameString = ESString::create("localName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrPrototypeObj, localNameString, localNameGetterFunction, nullptr);

    ESString* nameString = ESString::create("name");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrPrototypeObj, nameString, nameGetterFunction, nullptr);

    ESString* valueString = ESString::create("value");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrPrototypeObj, valueString, valueGetterFunction,
        valueSetterFunction);

    ESString* ownerElementString = ESString::create("ownerElement");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrPrototypeObj, ownerElementString, ownerElementGetterFunction,
        nullptr);

    ESString* specifiedString = ESString::create("specified");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        AttrPrototypeObj, specifiedString, specifiedGetterFunction, nullptr);

    return AttrFunction;
}

void Attr::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(fetchData(instance)->fnAttr()->protoType());

    postInit(instance);
}

bool Attr::isAttr() const
{
    return true;
}
}
