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

static ESValue commentFunction(ESVMInstance* instance)
{
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    if (arg0.isUndefined()) {
        arg0 = ESString::create("");
    }
    String* data0 = String::fromUTF8(arg0.toString()->utf8Data());
    Window* window = (Window*)instance->globalObject()->extraPointerData();
    Comment* comment = new Comment(window->document(), data0);

    return comment->scriptValue();
}

ESFunctionObject* bindingComment(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* CommentString = ESString::create("Comment");
    ESFunctionObject* CommentFunction = ESFunctionObject::create(
        nullptr, commentFunction, CommentString, 0, true, true);

    CommentFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);

    CommentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);

    CommentFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData()->protoType());

    CommentFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData());

    return CommentFunction;
}
}
