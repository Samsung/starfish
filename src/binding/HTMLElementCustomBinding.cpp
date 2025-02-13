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
