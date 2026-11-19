/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "StarfishConfig.h"

#ifdef STARFISH_ENABLE_CANVAS
#include "binding/ScriptBindingInstance.h"
#include "core/dom/canvas/OffscreenCanvas.h"
#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

static uint64_t toCanvasDimension(ExecutionStateRef* state, ValueRef* value)
{
    // Web IDL's [EnforceRange] uses the safely representable integer range
    // for an unsigned long long, after truncating towards zero.
    double dimension = std::trunc(value->toNumber(state));
    if (!std::isfinite(dimension) || dimension < 0 ||
        dimension > 9007199254740991.0) {
        state->throwException(TypeErrorObjectRef::create(
            state, StringRef::createFromASCII("Invalid canvas dimension")));
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return static_cast<uint64_t>(dimension);
}

ValueRef* offscreencanvasConstructor(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv,
                                     OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "OffscreenCanvas");
        THROW_EXCEPTION(msg);
    }
    if (argc < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argc);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "OffscreenCanvas", reason);
        THROW_EXCEPTION(msg);
    }

    auto width = toCanvasDimension(state, argv[0]);
    auto height = toCanvasDimension(state, argv[1]);
    auto* instance = fetchScriptBindingInstance(state->context());
    auto* result = new OffscreenCanvas(fetchExecutionContext(state->context()),
                                       width, height);
    if (newTarget.value() != instance->fnOffscreenCanvas()) {
        auto* prototype =
            newTarget->isFunctionObject()
                ? newTarget->asFunctionObject()->getFunctionPrototype(state)
                : newTarget->get(state, scriptStringPrototype(instance));
        result->scriptObject()->setPrototype(state, prototype);
    }
    return result->scriptValue();
}

ValueRef* widthOffscreenCanvasSetterFunction(ExecutionStateRef* state,
                                             ValueRef* thisValue, size_t argc,
                                             ValueRef** argv,
                                             bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(OffscreenCanvas);
    originalObj->setWidth(toCanvasDimension(state, argv[0]));
    return ValueRef::createUndefined();
}

ValueRef* heightOffscreenCanvasSetterFunction(ExecutionStateRef* state,
                                              ValueRef* thisValue, size_t argc,
                                              ValueRef** argv,
                                              bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(OffscreenCanvas);
    originalObj->setHeight(toCanvasDimension(state, argv[0]));
    return ValueRef::createUndefined();
}

} // namespace Starfish
#endif
