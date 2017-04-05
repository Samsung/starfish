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

static ESValue getComputedStyleFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0),
                     Node);
        // Node* obj =
        // (Node*)thisValue.asESPointer()->asESObject()
        //                               ->extraPointerData();
        Node* node = (Node*)instance->currentExecutionContext()
                         ->readArgument(0)
                         .asESPointer()
                         ->asESObject()
                         ->extraPointerData();
        ESValue pseudoElm =
            instance->currentExecutionContext()->readArgument(1);

        if (node->isNode() && pseudoElm.isNull()) {
            CSSStyleDeclaration* s = node->asNode()->getComputedStyle();
            if (s) {
                return s->scriptValue();
            }
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

ESFunctionObject* bindingWindow(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Window, fetchData(scriptBindingInstance)->m_eventTarget);
    fetchData(scriptBindingInstance)
        ->m_instance->globalObject()
        ->set__proto__(WindowFunction->protoType());
    fetchData(scriptBindingInstance)
        ->m_instance->globalObject()
        ->defineDataProperty(WindowString, true, false, true, WindowFunction);

#ifdef STARFISH_ENABLE_TEST
    WindowFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getComputedStyle"), true, true, true,
        ESFunctionObject::create(nullptr, getComputedStyleFunction,
                                 ESString::create("getComputedStyle"), 2,
                                 false));
#endif

    return WindowFunction;
}
}
