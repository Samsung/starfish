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
#include "dom/DOMTokenList.h"

namespace StarFish {

using namespace escargot;

ESValue addDOMTokenListFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, DOMTokenList);
    try {
        GCVector<String*> tokens;
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(i);
            ESString* argStr = argValue.toString();
            String* aa = toBrowserString(argStr);
            tokens.push_back(aa);
        }
        if (argCount > 0) {
            ((DOMTokenList*)thisValue.asESPointer()
                 ->asESObject()
                 ->extraPointerData())
                ->add(&tokens);
        }
        return ESValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}
}
