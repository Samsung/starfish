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

#include "dom/HTMLDivElement.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLDivElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLDivElementString = ESString::create("HTMLDivElement");
    ESFunctionObject* HTMLDivElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLDivElementString, 0, true, true);
    ESObject* HTMLDivElementPrototypeObj =
        HTMLDivElementFunction->protoType().asESPointer()->asESObject();
    HTMLDivElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLDivElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLDivElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLDivElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    return HTMLDivElementFunction;
}

void HTMLDivElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLDivElement()->protoType());

    postInit(instance);
}

bool HTMLDivElement::isHTMLDivElement() const
{
    return true;
}
}
