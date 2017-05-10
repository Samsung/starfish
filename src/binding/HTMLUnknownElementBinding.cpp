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

#include "dom/HTMLUnknownElement.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLUnknownElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLUnknownElementString = ESString::create("HTMLUnknownElement");
    ESFunctionObject* HTMLUnknownElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLUnknownElementString, 0, true, true);
    ESObject* HTMLUnknownElementPrototypeObj =
        HTMLUnknownElementFunction->protoType().asESPointer()->asESObject();
    HTMLUnknownElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLUnknownElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLUnknownElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLUnknownElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    return HTMLUnknownElementFunction;
}

void HTMLUnknownElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLUnknownElement()->protoType());

    postInit(instance);
}

bool HTMLUnknownElement::isHTMLUnknownElement() const
{
    return true;
}
}
