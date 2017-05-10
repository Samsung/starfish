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

#include "dom/Text.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue textConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "Text");
        THROW_EXCEPTION(msg);
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    Text* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 1)
    result = new Text(callWith, value0);
    return result->scriptValue();
}

// Implement for attributes
static ESValue wholeTextGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Text);
    // Declare native value (empty when type is void)
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
        nullptr, textConstructor, TextString, 0, true, true);
    ESObject* TextPrototypeObj =
        TextFunction->protoType().asESPointer()->asESObject();
    TextFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextPrototypeObj->forceNonVectorHiddenClass(false);
    TextPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData()->protoType());
    TextFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData());
    // Bind for attributes
    ESString* wholeTextString = ESString::create("wholeText");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextPrototypeObj, wholeTextString, wholeTextGetterFunction, nullptr);

    // Bind for functions

    return TextFunction;
}

void Text::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(fetchData(instance)->fnText()->protoType());

    postInit(instance);
}

bool Text::isText() const
{
    return true;
}
}
