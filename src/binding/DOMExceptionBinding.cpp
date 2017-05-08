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

#include "dom/DOMException.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue domexceptionConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMException");
        THROW_EXCEPTION(msg);
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    String* value1 = String::fromUTF8("Error");
    if (!arg1.isUndefinedOrNull()) {
        value1 = toBrowserString(arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    DOMException* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 2)
    result = new DOMException(callWith, value0, value1);
    return result->scriptValue();
}

// Implement for attributes
static ESValue codeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMException);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->code();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMException);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->name();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue messageGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMException);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->message();
    // Return ESValue from native value
    return toJSString(result);
}

ESFunctionObject* bindingDOMException(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMExceptionString = ESString::create("DOMException");
    ESFunctionObject* DOMExceptionFunction = ESFunctionObject::create(
        nullptr, domexceptionConstructor, DOMExceptionString, 0, true, true);
    ESObject* DOMExceptionPrototypeObj =
        DOMExceptionFunction->protoType().asESPointer()->asESObject();
    DOMExceptionFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMExceptionPrototypeObj->forceNonVectorHiddenClass(false);
    DOMExceptionPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                               ->m_instance->globalObject()
                                               ->errorPrototype());

    // Bind for constants
    ESString* INDEX_SIZE_ERRString = ESString::create("INDEX_SIZE_ERR");
    ESValue INDEX_SIZE_ERRValue = ESValue(1);
    DOMExceptionPrototypeObj->defineDataProperty(
        INDEX_SIZE_ERRString, false, true, false, INDEX_SIZE_ERRValue);

    DOMExceptionFunction->defineDataProperty(INDEX_SIZE_ERRString, false, true,
                                             false, INDEX_SIZE_ERRValue);

    ESString* HIERARCHY_REQUEST_ERRString =
        ESString::create("HIERARCHY_REQUEST_ERR");
    ESValue HIERARCHY_REQUEST_ERRValue = ESValue(3);
    DOMExceptionPrototypeObj->defineDataProperty(HIERARCHY_REQUEST_ERRString,
                                                 false, true, false,
                                                 HIERARCHY_REQUEST_ERRValue);

    DOMExceptionFunction->defineDataProperty(HIERARCHY_REQUEST_ERRString, false,
                                             true, false,
                                             HIERARCHY_REQUEST_ERRValue);

    ESString* WRONG_DOCUMENT_ERRString = ESString::create("WRONG_DOCUMENT_ERR");
    ESValue WRONG_DOCUMENT_ERRValue = ESValue(4);
    DOMExceptionPrototypeObj->defineDataProperty(
        WRONG_DOCUMENT_ERRString, false, true, false, WRONG_DOCUMENT_ERRValue);

    DOMExceptionFunction->defineDataProperty(
        WRONG_DOCUMENT_ERRString, false, true, false, WRONG_DOCUMENT_ERRValue);

    ESString* INVALID_CHARACTER_ERRString =
        ESString::create("INVALID_CHARACTER_ERR");
    ESValue INVALID_CHARACTER_ERRValue = ESValue(5);
    DOMExceptionPrototypeObj->defineDataProperty(INVALID_CHARACTER_ERRString,
                                                 false, true, false,
                                                 INVALID_CHARACTER_ERRValue);

    DOMExceptionFunction->defineDataProperty(INVALID_CHARACTER_ERRString, false,
                                             true, false,
                                             INVALID_CHARACTER_ERRValue);

    ESString* NO_MODIFICATION_ALLOWED_ERRString =
        ESString::create("NO_MODIFICATION_ALLOWED_ERR");
    ESValue NO_MODIFICATION_ALLOWED_ERRValue = ESValue(7);
    DOMExceptionPrototypeObj->defineDataProperty(
        NO_MODIFICATION_ALLOWED_ERRString, false, true, false,
        NO_MODIFICATION_ALLOWED_ERRValue);

    DOMExceptionFunction->defineDataProperty(NO_MODIFICATION_ALLOWED_ERRString,
                                             false, true, false,
                                             NO_MODIFICATION_ALLOWED_ERRValue);

    ESString* NOT_FOUND_ERRString = ESString::create("NOT_FOUND_ERR");
    ESValue NOT_FOUND_ERRValue = ESValue(8);
    DOMExceptionPrototypeObj->defineDataProperty(
        NOT_FOUND_ERRString, false, true, false, NOT_FOUND_ERRValue);

    DOMExceptionFunction->defineDataProperty(NOT_FOUND_ERRString, false, true,
                                             false, NOT_FOUND_ERRValue);

    ESString* NOT_SUPPORTED_ERRString = ESString::create("NOT_SUPPORTED_ERR");
    ESValue NOT_SUPPORTED_ERRValue = ESValue(9);
    DOMExceptionPrototypeObj->defineDataProperty(
        NOT_SUPPORTED_ERRString, false, true, false, NOT_SUPPORTED_ERRValue);

    DOMExceptionFunction->defineDataProperty(
        NOT_SUPPORTED_ERRString, false, true, false, NOT_SUPPORTED_ERRValue);

    ESString* INUSE_ATTRIBUTE_ERRString =
        ESString::create("INUSE_ATTRIBUTE_ERR");
    ESValue INUSE_ATTRIBUTE_ERRValue = ESValue(10);
    DOMExceptionPrototypeObj->defineDataProperty(INUSE_ATTRIBUTE_ERRString,
                                                 false, true, false,
                                                 INUSE_ATTRIBUTE_ERRValue);

    DOMExceptionFunction->defineDataProperty(INUSE_ATTRIBUTE_ERRString, false,
                                             true, false,
                                             INUSE_ATTRIBUTE_ERRValue);

    ESString* INVALID_STATE_ERRString = ESString::create("INVALID_STATE_ERR");
    ESValue INVALID_STATE_ERRValue = ESValue(11);
    DOMExceptionPrototypeObj->defineDataProperty(
        INVALID_STATE_ERRString, false, true, false, INVALID_STATE_ERRValue);

    DOMExceptionFunction->defineDataProperty(
        INVALID_STATE_ERRString, false, true, false, INVALID_STATE_ERRValue);

    ESString* SYNTAX_ERRString = ESString::create("SYNTAX_ERR");
    ESValue SYNTAX_ERRValue = ESValue(12);
    DOMExceptionPrototypeObj->defineDataProperty(SYNTAX_ERRString, false, true,
                                                 false, SYNTAX_ERRValue);

    DOMExceptionFunction->defineDataProperty(SYNTAX_ERRString, false, true,
                                             false, SYNTAX_ERRValue);

    ESString* INVALID_MODIFICATION_ERRString =
        ESString::create("INVALID_MODIFICATION_ERR");
    ESValue INVALID_MODIFICATION_ERRValue = ESValue(13);
    DOMExceptionPrototypeObj->defineDataProperty(INVALID_MODIFICATION_ERRString,
                                                 false, true, false,
                                                 INVALID_MODIFICATION_ERRValue);

    DOMExceptionFunction->defineDataProperty(INVALID_MODIFICATION_ERRString,
                                             false, true, false,
                                             INVALID_MODIFICATION_ERRValue);

    ESString* NAMESPACE_ERRString = ESString::create("NAMESPACE_ERR");
    ESValue NAMESPACE_ERRValue = ESValue(14);
    DOMExceptionPrototypeObj->defineDataProperty(
        NAMESPACE_ERRString, false, true, false, NAMESPACE_ERRValue);

    DOMExceptionFunction->defineDataProperty(NAMESPACE_ERRString, false, true,
                                             false, NAMESPACE_ERRValue);

    ESString* INVALID_ACCESS_ERRString = ESString::create("INVALID_ACCESS_ERR");
    ESValue INVALID_ACCESS_ERRValue = ESValue(15);
    DOMExceptionPrototypeObj->defineDataProperty(
        INVALID_ACCESS_ERRString, false, true, false, INVALID_ACCESS_ERRValue);

    DOMExceptionFunction->defineDataProperty(
        INVALID_ACCESS_ERRString, false, true, false, INVALID_ACCESS_ERRValue);

    ESString* SECURITY_ERRString = ESString::create("SECURITY_ERR");
    ESValue SECURITY_ERRValue = ESValue(18);
    DOMExceptionPrototypeObj->defineDataProperty(
        SECURITY_ERRString, false, true, false, SECURITY_ERRValue);

    DOMExceptionFunction->defineDataProperty(SECURITY_ERRString, false, true,
                                             false, SECURITY_ERRValue);

    ESString* NETWORK_ERRString = ESString::create("NETWORK_ERR");
    ESValue NETWORK_ERRValue = ESValue(19);
    DOMExceptionPrototypeObj->defineDataProperty(NETWORK_ERRString, false, true,
                                                 false, NETWORK_ERRValue);

    DOMExceptionFunction->defineDataProperty(NETWORK_ERRString, false, true,
                                             false, NETWORK_ERRValue);

    ESString* ABORT_ERRString = ESString::create("ABORT_ERR");
    ESValue ABORT_ERRValue = ESValue(20);
    DOMExceptionPrototypeObj->defineDataProperty(ABORT_ERRString, false, true,
                                                 false, ABORT_ERRValue);

    DOMExceptionFunction->defineDataProperty(ABORT_ERRString, false, true,
                                             false, ABORT_ERRValue);

    ESString* URL_MISMATCH_ERRString = ESString::create("URL_MISMATCH_ERR");
    ESValue URL_MISMATCH_ERRValue = ESValue(21);
    DOMExceptionPrototypeObj->defineDataProperty(
        URL_MISMATCH_ERRString, false, true, false, URL_MISMATCH_ERRValue);

    DOMExceptionFunction->defineDataProperty(
        URL_MISMATCH_ERRString, false, true, false, URL_MISMATCH_ERRValue);

    ESString* QUOTA_EXCEEDED_ERRString = ESString::create("QUOTA_EXCEEDED_ERR");
    ESValue QUOTA_EXCEEDED_ERRValue = ESValue(22);
    DOMExceptionPrototypeObj->defineDataProperty(
        QUOTA_EXCEEDED_ERRString, false, true, false, QUOTA_EXCEEDED_ERRValue);

    DOMExceptionFunction->defineDataProperty(
        QUOTA_EXCEEDED_ERRString, false, true, false, QUOTA_EXCEEDED_ERRValue);

    ESString* TIMEOUT_ERRString = ESString::create("TIMEOUT_ERR");
    ESValue TIMEOUT_ERRValue = ESValue(23);
    DOMExceptionPrototypeObj->defineDataProperty(TIMEOUT_ERRString, false, true,
                                                 false, TIMEOUT_ERRValue);

    DOMExceptionFunction->defineDataProperty(TIMEOUT_ERRString, false, true,
                                             false, TIMEOUT_ERRValue);

    ESString* INVALID_NODE_TYPE_ERRString =
        ESString::create("INVALID_NODE_TYPE_ERR");
    ESValue INVALID_NODE_TYPE_ERRValue = ESValue(24);
    DOMExceptionPrototypeObj->defineDataProperty(INVALID_NODE_TYPE_ERRString,
                                                 false, true, false,
                                                 INVALID_NODE_TYPE_ERRValue);

    DOMExceptionFunction->defineDataProperty(INVALID_NODE_TYPE_ERRString, false,
                                             true, false,
                                             INVALID_NODE_TYPE_ERRValue);

    ESString* DATA_CLONE_ERRString = ESString::create("DATA_CLONE_ERR");
    ESValue DATA_CLONE_ERRValue = ESValue(25);
    DOMExceptionPrototypeObj->defineDataProperty(
        DATA_CLONE_ERRString, false, true, false, DATA_CLONE_ERRValue);

    DOMExceptionFunction->defineDataProperty(DATA_CLONE_ERRString, false, true,
                                             false, DATA_CLONE_ERRValue);

    // Bind for attributes
    ESString* codeString = ESString::create("code");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionPrototypeObj, codeString, codeGetterFunction, nullptr);

    ESString* nameString = ESString::create("name");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionPrototypeObj, nameString, nameGetterFunction, nullptr);

    ESString* messageString = ESString::create("message");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionPrototypeObj, messageString, messageGetterFunction,
        nullptr);

    return DOMExceptionFunction;
}
}
