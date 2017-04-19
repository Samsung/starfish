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
#include "ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/HTMLTFootElement.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLTFootElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLTFootElementString = ESString::create("HTMLTFootElement");
    ESFunctionObject* HTMLTFootElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLTFootElementString, 1, true, true);
    HTMLTFootElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLTFootElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLTFootElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)->fnText()->protoType());
    HTMLTFootElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnText());

    return HTMLTFootElementFunction;
}
}
