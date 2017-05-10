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

#include "dom/HTMLPreElement.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLPreElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLPreElementString = ESString::create("HTMLPreElement");
    ESFunctionObject* HTMLPreElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLPreElementString, 0, true, true);
    ESObject* HTMLPreElementPrototypeObj =
        HTMLPreElementFunction->protoType().asESPointer()->asESObject();
    HTMLPreElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLPreElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLPreElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLPreElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    return HTMLPreElementFunction;
}

void HTMLPreElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLPreElement()->protoType());

    postInit(instance);
}

bool HTMLPreElement::isHTMLPreElement() const
{
    return true;
}
}
