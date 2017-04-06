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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue commentFunction(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (argCount > 0) {
        ESString* data = firstArg.toString();
        Comment* comment =
            new Comment((((Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData()))
                            ->document(),
                        String::fromUTF8(data->utf8Data()));
        return comment->scriptValue();
    }
    return ESValue();
}

ESFunctionObject* bindingComment(ScriptBindingInstance* scriptBindingInstance)
{
    /* 4.10 Interface Comment */
    auto comment = ESFunctionObject::create(
        NULL, commentFunction, ESString::create("Comment"), 0, true, true);
    comment->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    comment->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(
        false);
    comment->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->characterData()->protoType());
    comment->set__proto__(fetchData(scriptBindingInstance)->characterData());

    return comment;
}
}
