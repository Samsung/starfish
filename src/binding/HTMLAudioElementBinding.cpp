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
#if defined(STARFISH_ENABLE_MULTIMEDIA)
#include "dom/HTMLAudioElement.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue htmlaudioelementConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW,
                        "HTMLAudioElement");
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 1;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (arg0.isUndefined()) {
        validArgCount--;
    } else {
        value0 = toBrowserString(arg0);
    }
    HTMLAudioElement* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 0-1)
    if (validArgCount == 0) {
        result = new HTMLAudioElement(callWith);
    } else if (validArgCount == 1) {
        result = new HTMLAudioElement(callWith, value0);
    }
    return result->scriptValue();
}

ESFunctionObject* bindingHTMLAudioElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLAudioElementString = ESString::create("HTMLAudioElement");
    ESFunctionObject* HTMLAudioElementFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 HTMLAudioElementString, 0, true, true);
    ESObject* HTMLAudioElementPrototypeObj =
        HTMLAudioElementFunction->protoType().asESPointer()->asESObject();
    HTMLAudioElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLAudioElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLAudioElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLMediaElement()->protoType());
    HTMLAudioElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLMediaElement());

    return HTMLAudioElementFunction;
}

ESFunctionObject* bindingAudio(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* AudioString = ESString::create("Audio");

    ESFunctionObject* AudioFunction = ESFunctionObject::create(
        nullptr, htmlaudioelementConstructor, AudioString, 1, true, true);

    AudioFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    AudioFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    AudioFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLAudioElement()->protoType());
    AudioFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnHTMLAudioElement());
    return AudioFunction;
}

void HTMLAudioElement::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnHTMLAudioElement()->protoType());

    postInit(instance);
}

bool HTMLAudioElement::isHTMLAudioElement() const
{
    return true;
}
}
#endif
