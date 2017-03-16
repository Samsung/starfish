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

static ESValue dataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isCharacterData()) {
        return toJSString(nd->asCharacterData()->data());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue dataSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isCharacterData()) {
        if (v.isNull()) {
            nd->asCharacterData()->setData(
                toBrowserString(ESString::create("")));
            return ESValue();
        } else if (v.isUndefined()) {
            nd->asCharacterData()->setData(
                toBrowserString(ESString::create("undefined")));
            return ESValue();
        }
        nd->asCharacterData()->setData(toBrowserString(v));
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isCharacterData()) {
        if (nd->asCharacterData()->data()->isASCIIString()) {
            return ESValue(nd->asCharacterData()->length());
        } else {
            // TODO: measure length without converting
            return ESValue(
                toJSString(nd->asCharacterData()->data()).toString()->length());
        }
    }
    THROW_ILLEGAL_INVOCATION();
}

extern ESValue nextElementSiblingGetterFunction(ESVMInstance* instance);
extern ESValue previousElementSiblingGetterFunction(ESVMInstance* instance);
extern ESValue removeFunction(ESVMInstance* instance);

ESFunctionObject* bindingCharacterData(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        CharacterData, fetchData(scriptBindingInstance)->node());

    /* 4.9 Interface CharacterData */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(),
        ESString::create("data"), dataGetterFunction, dataSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

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
