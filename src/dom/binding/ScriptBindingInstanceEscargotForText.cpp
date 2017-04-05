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

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue textFunction(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (argCount > 0) {
        ESString* data = firstArg.toString();
        Text* text = new Text((((Window*)ESVMInstance::currentInstance()
                                    ->globalObject()
                                    ->extraPointerData()))
                                  ->document(),
                              String::fromUTF8(data->utf8Data()));
        return text->scriptValue();
    }
    return ESValue();
}

static ESValue wholeTextGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (!nd->isText()) {
        THROW_ILLEGAL_INVOCATION();
    }
    String* text = (nd->asText())->wholeText();
    return toJSString(text);
}

ESFunctionObject* bindingText(ScriptBindingInstance* scriptBindingInstance)
{
    /* 4.10 Interface Text */
    auto text = ESFunctionObject::create(
        NULL, textFunction, ESString::create("Text"), 0, true, true);
    text->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    text->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(
        false);
    text->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->characterData()->protoType());
    text->set__proto__(fetchData(scriptBindingInstance)->characterData());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        text->protoType().asESPointer()->asESObject(),
        ESString::create("wholeText"), wholeTextGetterFunction, nullptr);

    return text;
}
}
