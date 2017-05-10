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

#include "dom/DocumentType.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DocumentType);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->name();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue publicIdGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DocumentType);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->publicId();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue systemIdGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DocumentType);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->systemId();
    // Return ESValue from native value
    return toJSString(result);
}

// Implement for functions
static ESValue removeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DocumentType);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->remove();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingDocumentType(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DocumentTypeString = ESString::create("DocumentType");
    ESFunctionObject* DocumentTypeFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, DocumentTypeString, 0, true, true);
    ESObject* DocumentTypePrototypeObj =
        DocumentTypeFunction->protoType().asESPointer()->asESObject();
    DocumentTypeFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DocumentTypePrototypeObj->forceNonVectorHiddenClass(false);
    DocumentTypePrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());
    DocumentTypeFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnNode());

    // Bind for attributes
    ESString* nameString = ESString::create("name");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypePrototypeObj, nameString, nameGetterFunction, nullptr);

    ESString* publicIdString = ESString::create("publicId");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypePrototypeObj, publicIdString, publicIdGetterFunction,
        nullptr);

    ESString* systemIdString = ESString::create("systemId");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypePrototypeObj, systemIdString, systemIdGetterFunction,
        nullptr);

    // Bind for functions
    ESString* removeString = ESString::create("remove");
    ESFunctionObject* removeESFn = ESFunctionObject::create(
        nullptr, removeFunction, removeString, 0, false);
    DocumentTypePrototypeObj->defineDataProperty(removeString, true, true, true,
                                                 removeESFn);

    return DocumentTypeFunction;
}

void DocumentType::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDocumentType()->protoType());

    postInit(instance);
}

bool DocumentType::isDocumentType() const
{
    return true;
}
}
