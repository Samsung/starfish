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
#if defined(STARFISH_ENABLE_MULTI_PAGE)
#include "dom/HTMLAnchorElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
ESFunctionObject* bindingHTMLAnchorElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLAnchorElementString = ESString::create("HTMLAnchorElement");
    ESFunctionObject* HTMLAnchorElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLAnchorElementString, 0, true, true);
    ESObject* HTMLAnchorElementPrototypeObj =
        HTMLAnchorElementFunction->protoType().asESPointer()->asESObject();
    HTMLAnchorElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLAnchorElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLAnchorElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLAnchorElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes

    return HTMLAnchorElementFunction;
}

void HTMLAnchorElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLAnchorElement()->protoType());

    postInit(instance);
}

bool HTMLAnchorElement::isHTMLAnchorElement() const
{
    return true;
}
}
#endif
