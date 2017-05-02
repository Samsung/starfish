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

#include "dom/HTMLTableCaptionElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
ESFunctionObject* bindingHTMLTableCaptionElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLTableCaptionElementString =
        ESString::create("HTMLTableCaptionElement");
    ESFunctionObject* HTMLTableCaptionElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLTableCaptionElementString, 0, true, true);
    ESObject* HTMLTableCaptionElementPrototypeObj =
        HTMLTableCaptionElementFunction->protoType()
            .asESPointer()
            ->asESObject();
    HTMLTableCaptionElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLTableCaptionElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLTableCaptionElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLTableCaptionElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    return HTMLTableCaptionElementFunction;
}
}
