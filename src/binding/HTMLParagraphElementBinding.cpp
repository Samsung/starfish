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

#include "dom/HTMLParagraphElement.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLParagraphElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLParagraphElementString =
        ESString::create("HTMLParagraphElement");
    ESFunctionObject* HTMLParagraphElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLParagraphElementString, 1, true, true);
    HTMLParagraphElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLParagraphElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLParagraphElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLParagraphElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    return HTMLParagraphElementFunction;
}
}
