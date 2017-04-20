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

#include "dom/DOMException.h"
#include "dom/HTMLLinkElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue hrefGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLLinkElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->href();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hrefSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLLinkElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHref(value0);
    return ESValue();
}

static ESValue relGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLLinkElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->rel();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue relSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLLinkElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setRel(value0);
    return ESValue();
}

static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLLinkElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->type();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue typeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLLinkElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setType(value0);
    return ESValue();
}

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
        hrefString, hrefGetterFunction, hrefSetterFunction);

    ESString* relString = ESString::create("rel");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        relString, relGetterFunction, relSetterFunction);

    ESString* typeString = ESString::create("type");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        typeString, typeGetterFunction, typeSetterFunction);

    return HTMLLinkElementFunction;
}
}
