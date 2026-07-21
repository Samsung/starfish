/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLCustomElement.h"
#include "core/dom/CustomElementRegistry.h"
#include "core/page/Window.h"
#include "binding/ScriptBindingInstance.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

ValueRef* htmlelementConstructor(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv,
                                 OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "HTMLElement");
        THROW_EXCEPTION(msg);
    }

    Window* window = fetchWindow(state->context());
    CustomElementRegistry* customElement = window->customElements();
    auto data = customElement->find(newTarget.value());
    if (data) {
        // https://html.spec.whatwg.org/multipage/dom.html#htmlconstructor
        // Step 6-7: Check the construction stack of the definition.
        //   - Empty stack: this is a `new` call → create a fresh element.
        //   - Top is an element: mark as "already constructed" and return it.
        //   - Top is "already constructed" (nullptr): throw TypeError.
        HTMLCustomElement* stackTop =
            customElement->peekConstructionStack(data.value());
        if (stackTop) {
            // Mark top as "already constructed" and return the element.
            // This is the upgrade path: the upgrade algorithm pushed this
            // element, so super() must return it, not create a new one.
            customElement->markConstructionStackAlreadyConstructed(
                data.value());

            // Set prototype from newTarget (spec step 10-11)
            auto bindingInstance = fetchScriptBindingInstance(state->context());
            StringRef* prototypeString = scriptStringPrototype(bindingInstance);
            ValueRef* proto = ValueRef::createUndefined();
            if (newTarget->isFunctionObject()) {
                proto =
                    newTarget->asFunctionObject()->getFunctionPrototype(state);
            } else {
                proto = newTarget->get(state, prototypeString);
            }
            stackTop->scriptObject()->setPrototype(state, proto);
            return stackTop->scriptObject();
        }

        // Stack is empty OR top is "already constructed" marker.
        // If the stack is non-empty, the top is the nullptr sentinel → throw.
        // Otherwise (empty stack), proceed to create a new element (the `new`
        // path).
        if (!customElement->isConstructionStackEmpty(data.value())) {
            // Top is "already constructed" — constructor called super() twice
            COMPOSE_MESSAGE(msg, ILLEGAL_INVOKE);
            THROW_EXCEPTION(msg);
        }

        // Construction stack is empty — this is a direct `new` invocation.
        bool isDecendentOfHTMLElement = false;

        auto bindingInstance = fetchScriptBindingInstance(state->context());
        FunctionObjectRef* htmlElement = bindingInstance->fnHTMLElement();
        ObjectRef* htmlElementPrototype =
            htmlElement->getFunctionPrototype(state)->asObject();

        StringRef* prototypeString = scriptStringPrototype(bindingInstance);
        ValueRef* target = newTarget.value()->get(state, prototypeString);

        while (target && target->isObject()) {
            if (target == htmlElementPrototype) {
                isDecendentOfHTMLElement = true;
                break;
            }
            auto proto = target->asObject()->getPrototypeObject(state);
            if (proto) {
                target = proto.value();
            } else {
                target = nullptr;
            }
        }

        if (isDecendentOfHTMLElement) {
            auto obj = customElement
                           ->createCustomElement(window->document(),
                                                 data.value(), false)
                           ->scriptObject();

            ValueRef* proto = ValueRef::createUndefined();
            if (newTarget->isFunctionObject()) {
                proto =
                    newTarget->asFunctionObject()->getFunctionPrototype(state);
            } else {
                proto = newTarget->get(state, prototypeString);
            }
            obj->setPrototype(state, proto);
            return obj;
        }
    }

    COMPOSE_MESSAGE(msg, ILLEGAL_INVOKE);
    THROW_EXCEPTION(msg);
}

} // namespace Starfish
