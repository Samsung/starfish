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

ESFunctionObject* bindingDocumentType(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        DocumentType, fetchData(scriptBindingInstance)->node());

    ESFunctionObject* removeFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();
            Node* p = obj->parentNode();
            if (p == nullptr) {
                return ESValue(ESValue::ESUndefined);
            }
            obj->remove();
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("remove"), 0, false);
    DocumentTypeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("remove"), false, false, false,
                             removeFunction);

    /* 4.7 Interface DocumentType */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("name"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            String* s = originalObj->nodeName();
            return toJSString(s);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("publicId"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (!nd->isDocumentType()) {
                THROW_ILLEGAL_INVOCATION();
            }
            String* s = nd->asDocumentType()->publicId();
            return toJSString(s);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("systemId"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (!nd->isDocumentType()) {
                THROW_ILLEGAL_INVOCATION();
            }
            String* s = nd->asDocumentType()->systemId();
            return toJSString(s);
        },
        nullptr);

    return DocumentTypeFunction;
}
}
