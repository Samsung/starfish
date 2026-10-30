/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#include "StarfishConfig.h"
#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/modules/webaudio/OfflineAudioContext.h"
#include <EscargotPublic.h>
#include <cmath>

using namespace Escargot;

namespace Starfish {

OfflineAudioContextOptions toOfflineAudioContextOptionsFromValueRef(
    ExecutionStateRef* state, ValueRef* from);

ValueRef* offlineaudiocontextConstructor(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         ValueRef** argv,
                                         OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW,
                        "OfflineAudioContext");
        THROW_EXCEPTION(msg);
    }
    if (!argc) {
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", "0");
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "OfflineAudioContext",
                        reason);
        THROW_EXCEPTION(msg);
    }

    ExecutionContext* context = fetchExecutionContext(state->context());
    OfflineAudioContext* result = nullptr;
    try {
        if (argc < 3) {
            // The generated converter throws TypeError for a missing
            // required length or sampleRate.
            OfflineAudioContextOptions options =
                toOfflineAudioContextOptionsFromValueRef(state, argv[0]);
            result = new OfflineAudioContext(context, options);
        } else {
            uint32_t channels = argv[0]->toUint32(state);
            uint32_t length = argv[1]->toUint32(state);
            double sampleRate = argv[2]->toNumber(state);
            result =
                new OfflineAudioContext(context, channels, length, sampleRate);
        }
    } catch (DOMException* error) {
        state->throwException(error->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    ScriptBindingInstance* instance =
        fetchScriptBindingInstance(state->context());
    if (newTarget.value() != instance->fnOfflineAudioContext()) {
        ValueRef* prototype = ValueRef::createUndefined();
        if (newTarget->isFunctionObject()) {
            prototype =
                newTarget->asFunctionObject()->getFunctionPrototype(state);
        } else {
            prototype = newTarget->get(state, scriptStringPrototype(instance));
        }
        result->scriptObject()->setPrototype(state, prototype);
    }
    return result->scriptValue();
}

// https://webidl.spec.whatwg.org/#js-operations
// suspend() returns a promise, so a failed argument conversion (a missing
// or non-finite suspendTime, or a throwing valueOf) rejects it instead of
// throwing.
ValueRef* suspendOfflineAudioContextFunction(ExecutionStateRef* state,
                                             ValueRef* thisValue, size_t argc,
                                             ValueRef** argv,
                                             bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(OfflineAudioContext);
    auto converted = Evaluator::execute(
        state->context(),
        [](ExecutionStateRef* state, ValueRef* value) -> ValueRef* {
            return ValueRef::create(value->toNumber(state));
        },
        argc ? argv[0] : ValueRef::createUndefined());
    ValueRef* error = nullptr;
    if (converted.error.hasValue()) {
        error = converted.error.value();
    } else if (!std::isfinite(converted.result->asNumber())) {
        error = (new DOMException(originalObj->executionContext(),
                                  DOMException::SCRIPT_TYPE_ERR,
                                  "suspendTime must be a finite number"))
                    ->scriptValue();
    }
    if (error) {
        Promise* promise =
            new Promise(fetchScriptBindingInstance(state->context()));
        promise->reject(error);
        return promise->scriptValue();
    }
    return originalObj->suspend(converted.result->asNumber())->scriptValue();
}

} // namespace Starfish
#endif
