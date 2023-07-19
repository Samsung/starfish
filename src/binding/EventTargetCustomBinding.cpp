/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/EventTargetWithExecutionContext.h"
#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

ValueRef* eventtargetConstructor(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv,
                                 OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "EventTarget");
        THROW_EXCEPTION(msg);
    }

    EventTargetWithExecutionContext* eventTargetWithExecutionContext = nullptr;
    ExecutionContext* exeuctionContext =
        fetchExecutionContext(state->context());

    eventTargetWithExecutionContext =
        new EventTargetWithExecutionContext(exeuctionContext);

    if (newTarget.value() !=
        fetchScriptBindingInstance(state->context())->fnEventTarget()) {
        ValueRef* proto = ValueRef::createUndefined();
        if (newTarget->isFunctionObject()) {
            proto = newTarget->asFunctionObject()->getFunctionPrototype(state);
        } else {
            proto =
                newTarget->get(state, StringRef::createFromASCII("prototype"));
        }
        eventTargetWithExecutionContext->scriptObject()->setPrototype(state,
                                                                      proto);
    }

    return eventTargetWithExecutionContext->scriptValue();
}

} // namespace Starfish
