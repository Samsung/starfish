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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMException);
    return ESString::create(originalObj->name());
}

static ESValue messageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMException);
    return toJSString(originalObj->message());
}

static ESValue codeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMException);
    return ESValue(originalObj->code());
}

ESFunctionObject* bindingDOMException(
    ScriptBindingInstance* scriptBindingInstance)
{
    /* DOM Exception */
    DEFINE_FUNCTION(DOMException, fetchData(scriptBindingInstance)
                                      ->m_instance->globalObject()
                                      ->objectPrototype());
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->errorPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(),
        fetchData(scriptBindingInstance)->m_instance->strings().name,
        nameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(),
        fetchData(scriptBindingInstance)->m_instance->strings().message,
        messageGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(),
        ESString::create("code"), codeGetterFunction, nullptr);

    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INDEX_SIZE_ERR"), false, false, false, ESValue(1));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("HIERARCHY_REQUEST_ERR"), false, false, false,
        ESValue(3));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("WRONG_DOCUMENT_ERR"), false, false, false,
        ESValue(4));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INVALID_CHARACTER_ERR"), false, false, false,
        ESValue(5));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("NO_MODIFICATION_ALLOWED_ERR"), false, false, false,
        ESValue(7));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("NOT_FOUND_ERR"), false, false, false, ESValue(8));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("NOT_SUPPORTED_ERR"), false, false, false, ESValue(9));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INUSE_ATTRIBUTE_ERR"), false, false, false,
        ESValue(10));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INVALID_STATE_ERR"), false, false, false,
        ESValue(11));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("SYNTAX_ERR"), false, false, false, ESValue(12));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INVALID_MODIFICATION_ERR"), false, false, false,
        ESValue(13));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("NAMESPACE_ERR"), false, false, false, ESValue(14));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INVALID_ACCESS_ERR"), false, false, false,
        ESValue(15));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("SECURITY_ERR"), false, false, false, ESValue(18));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("NETWORK_ERR"), false, false, false, ESValue(19));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("ABORT_ERR"), false, false, false, ESValue(20));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("URL_MISMATCH_ERR"), false, false, false, ESValue(21));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("QUOTA_EXCEEDED_ERR"), false, false, false,
        ESValue(22));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("TIMEOUT_ERR"), false, false, false, ESValue(23));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("INVALID_NODE_TYPE_ERR"), false, false, false,
        ESValue(24));
    DOMExceptionFunction->asESObject()->defineDataProperty(
        ESString::create("DATA_CLONE_ERR"), false, false, false, ESValue(25));

    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INDEX_SIZE_ERR"), false, false,
                             false, ESValue(1));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("HIERARCHY_REQUEST_ERR"), false,
                             false, false, ESValue(3));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("WRONG_DOCUMENT_ERR"), false,
                             false, false, ESValue(4));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INVALID_CHARACTER_ERR"), false,
                             false, false, ESValue(5));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("NO_MODIFICATION_ALLOWED_ERR"),
                             false, false, false, ESValue(7));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("NOT_FOUND_ERR"), false, false,
                             false, ESValue(8));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("NOT_SUPPORTED_ERR"), false,
                             false, false, ESValue(9));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INUSE_ATTRIBUTE_ERR"), false,
                             false, false, ESValue(10));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INVALID_STATE_ERR"), false,
                             false, false, ESValue(11));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("SYNTAX_ERR"), false, false,
                             false, ESValue(12));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INVALID_MODIFICATION_ERR"),
                             false, false, false, ESValue(13));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("NAMESPACE_ERR"), false, false,
                             false, ESValue(14));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INVALID_ACCESS_ERR"), false,
                             false, false, ESValue(15));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("SECURITY_ERR"), false, false,
                             false, ESValue(18));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("NETWORK_ERR"), false, false,
                             false, ESValue(19));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("ABORT_ERR"), false, false, false,
                             ESValue(20));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("URL_MISMATCH_ERR"), false, false,
                             false, ESValue(21));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("QUOTA_EXCEEDED_ERR"), false,
                             false, false, ESValue(22));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("TIMEOUT_ERR"), false, false,
                             false, ESValue(23));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("INVALID_NODE_TYPE_ERR"), false,
                             false, false, ESValue(24));
    DOMExceptionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("DATA_CLONE_ERR"), false, false,
                             false, ESValue(25));

    return DOMExceptionFunction;
}
}
