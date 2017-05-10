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

#include "dom/HTMLBRElement.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLBRElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLBRElementString = ESString::create("HTMLBRElement");
    ESFunctionObject* HTMLBRElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLBRElementString, 0, true, true);
    ESObject* HTMLBRElementPrototypeObj =
        HTMLBRElementFunction->protoType().asESPointer()->asESObject();
    HTMLBRElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLBRElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLBRElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLBRElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    return HTMLBRElementFunction;
}

void HTMLBRElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLBRElement()->protoType());

    postInit(instance);
}

bool HTMLBRElement::isHTMLBRElement() const
{
    return true;
}
}
