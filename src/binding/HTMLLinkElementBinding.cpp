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

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
extern ESValue hrefHTMLLinkElementGetterFunction(ESVMInstance* instance);

extern ESValue hrefHTMLLinkElementSetterFunction(ESVMInstance* instance);

extern ESValue relHTMLLinkElementGetterFunction(ESVMInstance* instance);

extern ESValue relHTMLLinkElementSetterFunction(ESVMInstance* instance);

extern ESValue typeHTMLLinkElementGetterFunction(ESVMInstance* instance);

extern ESValue typeHTMLLinkElementSetterFunction(ESVMInstance* instance);

ESFunctionObject* bindingHTMLLinkElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLLinkElementString = ESString::create("HTMLLinkElement");
    ESFunctionObject* HTMLLinkElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLLinkElementString, 1, true, true);
    HTMLLinkElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLLinkElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLLinkElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLLinkElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* hrefString = ESString::create("href");

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        hrefString, hrefHTMLLinkElementGetterFunction,
        hrefHTMLLinkElementSetterFunction);

    ESString* relString = ESString::create("rel");

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        relString, relHTMLLinkElementGetterFunction,
        relHTMLLinkElementSetterFunction);

    ESString* typeString = ESString::create("type");

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        typeString, typeHTMLLinkElementGetterFunction,
        typeHTMLLinkElementSetterFunction);

    return HTMLLinkElementFunction;
}
}
