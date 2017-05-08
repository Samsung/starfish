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

#include "dom/CSSStyleDeclaration.h"
#include "dom/DOMException.h"
#include "dom/Node.h"
#include "platform/window/Window.h"

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
    // Bind for constructor
    ESString* WindowString = ESString::create("Window");
    ESFunctionObject* WindowFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, WindowString, 0, true, true);
    ESObject* WindowPrototypeObj =
        WindowFunction->protoType().asESPointer()->asESObject();
    WindowFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    WindowPrototypeObj->forceNonVectorHiddenClass(false);
    WindowPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    WindowFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());
/*
fetchData(scriptBindingInstance)
    ->m_instance->globalObject()
    ->set__proto__(WindowFunction->protoType());
fetchData(scriptBindingInstance)
    ->m_instance->globalObject()
    ->defineDataProperty(WindowString, true, false, true, WindowFunction);
*/
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
