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

#include "dom/Comment.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue commentConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "Comment");
        THROW_EXCEPTION(msg);
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    Comment* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 1)
    result = new Comment(callWith, value0);
    return result->scriptValue();
}

ESFunctionObject* bindingComment(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CommentString = ESString::create("Comment");
    ESFunctionObject* CommentFunction = ESFunctionObject::create(
        nullptr, commentConstructor, CommentString, 0, true, true);
    ESObject* CommentPrototypeObj =
        CommentFunction->protoType().asESPointer()->asESObject();
    CommentFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CommentPrototypeObj->forceNonVectorHiddenClass(false);
    CommentPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData()->protoType());
    CommentFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData());

    return CommentFunction;
}

void Comment::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(fetchData(instance)->fnComment()->protoType());

    postInit(instance);
}

bool Comment::isComment() const
{
    return true;
}
}
