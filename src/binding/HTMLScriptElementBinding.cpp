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

#include "dom/DOMException.h"
#include "dom/HTMLScriptElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue srcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->src();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue srcSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSrc(value0);
    return ESValue();
}

static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->type();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue typeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setType(value0);
    return ESValue();
}

static ESValue charsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->charset();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue charsetSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setCharset(value0);
    return ESValue();
}

static ESValue textGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->text();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue textSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setText(value0);
    return ESValue();
}

ESFunctionObject* bindingHTMLScriptElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLScriptElementString = ESString::create("HTMLScriptElement");
    ESFunctionObject* HTMLScriptElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLScriptElementString, 1, true, true);
    HTMLScriptElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLScriptElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    HTMLScriptElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            fetchData(scriptBindingInstance)->fnHTMLElement()->protoType());
    HTMLScriptElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLElement());

    // Bind for attributes
    ESString* srcString = ESString::create("src");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        srcString, srcGetterFunction, srcSetterFunction);

    ESString* typeString = ESString::create("type");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        typeString, typeGetterFunction, typeSetterFunction);

    ESString* charsetString = ESString::create("charset");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        charsetString, charsetGetterFunction, charsetSetterFunction);

    ESString* textString = ESString::create("text");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        textString, textGetterFunction, textSetterFunction);

    return HTMLScriptElementFunction;
}
}
