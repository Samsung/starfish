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

#include "dom/HTMLDocument.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
// Implement for functions
ESFunctionObject* bindingHTMLDocument(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLDocumentString = ESString::create("HTMLDocument");
    ESFunctionObject* HTMLDocumentFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, HTMLDocumentString, 1, true, true);
    HTMLDocumentFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLDocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLDocumentFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnDocument()->protoType());
    HTMLDocumentFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnDocument());
    ESObject* HTMLDocumentObj =
        HTMLDocumentFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    // Bind for functions
    return HTMLDocumentFunction;
}
}
