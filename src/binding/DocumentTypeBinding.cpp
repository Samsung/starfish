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

#include "dom/DocumentType.h"
#include "dom/DOMException.h"

namespace StarFish {

using namespace escargot;

extern ESValue removeFunction(ESVMInstance* instance);
static ESValue nameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    String* s = originalObj->nodeName();
    return toJSString(s);
}

static ESValue publicIdGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (!nd->isDocumentType()) {
        THROW_ILLEGAL_INVOCATION();
    }
    String* s = nd->asDocumentType()->publicId();
    return toJSString(s);
}

static ESValue systemIdGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (!nd->isDocumentType()) {
        THROW_ILLEGAL_INVOCATION();
    }
    String* s = nd->asDocumentType()->systemId();
    return toJSString(s);
}

ESFunctionObject* bindingDocumentType(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        DocumentType, fetchData(scriptBindingInstance)->fnNode());

    DocumentTypeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), false, false, false,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 0, false));

    /* 4.7 Interface DocumentType */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("name"), nameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("publicId"), publicIdGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("systemId"), systemIdGetterFunction, nullptr);

    return DocumentTypeFunction;
}
}
