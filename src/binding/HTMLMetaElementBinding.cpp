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

#include "dom/HTMLMetaElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
ESFunctionObject* bindingHTMLMetaElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLMetaElementString = ESString::create("HTMLMetaElement");
    ESFunctionObject* HTMLMetaElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLMetaElementString, 0, true, true);
    ESObject* HTMLMetaElementPrototypeObj =
        HTMLMetaElementFunction->protoType().asESPointer()->asESObject();
    HTMLMetaElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLMetaElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLMetaElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLMetaElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes

    return HTMLMetaElementFunction;
}

void HTMLMetaElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLMetaElement()->protoType());

    postInit(instance);
}

bool HTMLMetaElement::isHTMLMetaElement() const
{
    return true;
}
}
