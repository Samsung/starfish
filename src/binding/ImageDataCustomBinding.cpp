/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#ifdef STARFISH_ENABLE_CANVAS
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/canvas/ImageData.h"
#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {
ValueRef* imagedataConstructor(ExecutionStateRef* state, ValueRef* thisValue,
                               size_t argc, ValueRef** argv,
                               OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "ImageData");
        THROW_EXCEPTION(msg);
    }
    size_t argCount = argc;
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "ImageData", reason);
        THROW_EXCEPTION(msg);
    }

    STARFISH_ASSERT(argv != nullptr);

    size_t validArgCount = 3;
    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    ValueRef* arg2 = (argc > 2) ? argv[2] : ValueRef::createUndefined();
    // Handle argument arg2
    uint32_t value2;
    if (arg2->isUndefined()) {
        validArgCount--;
    } else {
        value2 = arg2->toUint32(state);
    }
    // Handle argument arg1
    uint32_t value1;
    value1 = arg1->toUint32(state);

    ImageData* result = nullptr;
    ExecutionContext* callWith = fetchExecutionContext(state->context());
    try {
        // Call native function (nargs: 2-3)
        if (arg0->isObject() && arg0->asObject()->isUint8ClampedArrayObject()) {
            // Handle argument arg0
            ScriptUint8ClampedArray value0 =
                arg0->asObject()->asUint8ClampedArrayObject();

            if (validArgCount == 2) {
                result = new ImageData(callWith, value0, value1,
                                       Optional<uint32_t>());
            } else if (validArgCount == 3) {
                result = new ImageData(callWith, value0, value1, value2);
            }
        } else {
            // Handle argument arg0
            uint32_t value0;
            value0 = arg0->toUint32(state);
            result = new ImageData(callWith, value0, value1);
        }
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (newTarget.value() !=
        fetchScriptBindingInstance(state->context())->fnImageData()) {
        ValueRef* proto = ValueRef::createUndefined();
        if (newTarget->isFunctionObject()) {
            proto = newTarget->asFunctionObject()->getFunctionPrototype(state);
        } else {
            proto = newTarget->get(
                state, scriptStringPrototype(
                           fetchScriptBindingInstance(state->context())));
        }
        result->scriptObject()->setPrototype(state, proto);
    }

    return result->scriptValue();
}
} // namespace Starfish
#endif
