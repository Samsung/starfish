/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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
#include "core/util/URLSearchParams.h"
#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {
ValueRef* urlsearchparamsConstructor(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv,
                                     OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "URLSearchParams");
        THROW_EXCEPTION(msg);
    }

    // Handle argument arg0
    ValueRef* arg0 = (argc > 0) ? argv[0] : ValueRef::createUndefined();

    URLSearchParams* result = nullptr;
    ExecutionContext* callWith = fetchExecutionContext(state->context());
    ScriptBindingInstance* instance = fetchScriptBindingInstance(state->context());

    // Handles sequence<sequence<String>>
    try {
        if (arg0->isObject()) {
            ValueRef* lengthObject = arg0->asObject()->get(
                state, scriptStringLength(instance));

            if (lengthObject->isUndefined()) {
                result = new URLSearchParams(callWith);
                return result->scriptValue();
            }

            int outerListLength = lengthObject->toNumber(state);
            GCVector<GCVector<String*>> value;

            for (int i = 0; i < outerListLength; i++) {
                ValueRef* innerList =
                    arg0->asObject()->get(state, ValueRef::create(i));

                if (innerList->isObject()) {
                    ValueRef* innerLengthObject = innerList->asObject()->get(
                        state, scriptStringLength(instance));
                    if (innerLengthObject->isUndefined()) {
                        throw new DOMException(callWith,
                                               DOMException::SCRIPT_TYPE_ERR,
                                               "expect a sequence");
                    }

                    int innerListLength = innerLengthObject->toNumber(state);
                    GCVector<String*> valueInner;
                    for (int j = 0; j < innerListLength; j++) {
                        ValueRef* innerItem = innerList->asObject()->get(
                            state, ValueRef::create(j));
                        if (innerItem->isUndefined()) {
                            continue;
                        }

                        String* item = toBrowserString(state, innerItem);
                        valueInner.push_back(item);
                    }
                    value.push_back(valueInner);
                }
            }

            result = new URLSearchParams(callWith, value);
        } else if (arg0->isString()) {
            String* arg = toBrowserString(state, arg0);
            result = new URLSearchParams(callWith, arg);
        }
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (result == nullptr) {
        result = new URLSearchParams(callWith);
    }

    if (newTarget.value() !=
        fetchScriptBindingInstance(state->context())->fnURLSearchParams()) {
        ValueRef* proto = ValueRef::createUndefined();
        if (newTarget->isFunctionObject()) {
            proto = newTarget->asFunctionObject()->getFunctionPrototype(state);
        } else {
            proto =
                newTarget->get(state, scriptStringPrototype(instance));
        }
        result->scriptObject()->setPrototype(state, proto);
    }

    return result->scriptValue();
}
} // namespace Starfish
