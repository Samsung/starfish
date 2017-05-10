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

#include "dom/XMLDocument.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingXMLDocument(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* XMLDocumentString = ESString::create("XMLDocument");
    ESFunctionObject* XMLDocumentFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, XMLDocumentString, 0, true, true);
    ESObject* XMLDocumentPrototypeObj =
        XMLDocumentFunction->protoType().asESPointer()->asESObject();
    XMLDocumentFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    XMLDocumentPrototypeObj->forceNonVectorHiddenClass(false);
    XMLDocumentPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnDocument()->protoType());
    XMLDocumentFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnDocument());

    return XMLDocumentFunction;
}

void XMLDocument::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnXMLDocument()->protoType());

    postInit(instance);
}

bool XMLDocument::isXMLDocument() const
{
    return true;
}
}
