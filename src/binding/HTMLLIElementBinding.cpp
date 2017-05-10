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

#include "dom/HTMLLIElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
ESFunctionObject* bindingHTMLLIElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLLIElementString = ESString::create("HTMLLIElement");
    ESFunctionObject* HTMLLIElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLLIElementString, 0, true, true);
    ESObject* HTMLLIElementPrototypeObj =
        HTMLLIElementFunction->protoType().asESPointer()->asESObject();
    HTMLLIElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLLIElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLLIElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLLIElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes

    return HTMLLIElementFunction;
}

void HTMLLIElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLLIElement()->protoType());

    postInit(instance);
}

bool HTMLLIElement::isHTMLLIElement() const
{
    return true;
}
}
