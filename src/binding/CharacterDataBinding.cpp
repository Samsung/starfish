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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/CharacterData.h"
#include "dom/DOMException.h"
#include "dom/Element.h"

namespace StarFish {

using namespace escargot;
// Implement for attributes
static ESValue dataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    String* v = originalObj->data();
    return toJSString(v);
}

static ESValue dataSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    ESValue originalV = instance->currentExecutionContext()->readArgument(0);
    String* v;
    if (originalV.isNull()) {
        v = String::emptyString;
    } else {
        v = toBrowserString(originalV);
    }
    originalObj->setData(v);
    return ESValue();
}

extern ESValue lengthCharacterDataGetterFunction(ESVMInstance* instance);

static ESValue previousElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
    Element* result = nullptr;
    result = originalObj->nextElementSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

extern ESValue removeFunction(ESVMInstance* instance);

ESFunctionObject* bindingCharacterData(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        CharacterData, fetchData(scriptBindingInstance)->fnNode());

    /* 4.9 Interface CharacterData */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(),
        ESString::create("data"), dataGetterFunction, dataSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthCharacterDataGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nextElementSibling"),
        nextElementSiblingGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(),
        ESString::create("previousElementSibling"),
        previousElementSiblingGetterFunction, nullptr);

    CharacterDataFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), false, false, false,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 0, false));

    return CharacterDataFunction;
}
}
