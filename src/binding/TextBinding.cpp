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
#include "dom/Text.h"

namespace StarFish {

using namespace escargot;

static ESValue textFunction(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (argCount > 0) {
        ESString* data = firstArg.toString();
        Document* document = fetchDocument(instance);
        Text* text = new Text(document, String::fromUTF8(data->utf8Data()));
        return text->scriptValue();
    }
    return ESValue();
}

// Implement for attributes
static ESValue wholeTextGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Text);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->wholeText();
    // Return ESValue from native value
    return toJSString(result);
}

// Implement for functions
ESFunctionObject* bindingText(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TextString = ESString::create("Text");
    ESFunctionObject* TextFunction = ESFunctionObject::create(
        nullptr, textFunction, TextString, 0, true, true);
    TextFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    TextFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData()->protoType());
    TextFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData());
    ESObject* TextPrototypeObj =
        TextFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* wholeTextString = ESString::create("wholeText");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextPrototypeObj, wholeTextString, wholeTextGetterFunction, nullptr);

    // Bind for functions
    return TextFunction;
}
}
