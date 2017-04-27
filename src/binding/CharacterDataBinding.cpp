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

#include "dom/CharacterData.h"
#include "dom/Element.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue dataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->data();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue dataSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isNull()) {
        value0 = toBrowserString(arg0);
    }
    originalObj->setData(value0);
    return ESValue();
}

extern ESValue lengthCharacterDataGetterFunction(ESVMInstance* instance);

static ESValue previousElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->previousElementSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue nextElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->nextElementSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

// Implement for functions
static ESValue removeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->remove();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingCharacterData(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CharacterDataString = ESString::create("CharacterData");
    ESFunctionObject* CharacterDataFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 CharacterDataString, 0, true, true);
    ESObject* CharacterDataPrototypeObj =
        CharacterDataFunction->protoType().asESPointer()->asESObject();
    CharacterDataFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CharacterDataPrototypeObj->forceNonVectorHiddenClass(false);
    CharacterDataPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());
    CharacterDataFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnNode());

    // Bind for attributes
    ESString* dataString = ESString::create("data");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataPrototypeObj, dataString, dataGetterFunction,
        dataSetterFunction);

    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataPrototypeObj, lengthString,
        lengthCharacterDataGetterFunction, nullptr);

    ESString* previousElementSiblingString =
        ESString::create("previousElementSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataPrototypeObj, previousElementSiblingString,
        previousElementSiblingGetterFunction, nullptr);

    ESString* nextElementSiblingString = ESString::create("nextElementSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataPrototypeObj, nextElementSiblingString,
        nextElementSiblingGetterFunction, nullptr);

    // Bind for functions
    ESString* removeString = ESString::create("remove");
    ESFunctionObject* removeESFn = ESFunctionObject::create(
        nullptr, removeFunction, removeString, 0, false);
    CharacterDataPrototypeObj->defineDataProperty(removeString, true, true,
                                                  true, removeESFn);

    return CharacterDataFunction;
}
}
