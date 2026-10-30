/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioWorkletGlobalScope.h"
#include "core/modules/webaudio/AudioWorkletProcessor.h"

#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

ScriptBindingInstance* AudioWorkletProcessor::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audioworkletprocessor-audioworkletprocessor
// Takes the port from the global scope's pending processor construction
// data, so only the construction the AudioWorkletNode started succeeds, once,
// whether by `new` or by super().
ValueRef* audioworkletprocessorConstructor(ExecutionStateRef* state,
                                           ValueRef* thisValue, size_t argc,
                                           ValueRef** argv,
                                           OptionalRef<ObjectRef> newTarget)
{
    if (!newTarget) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW,
                        "AudioWorkletProcessor");
        THROW_EXCEPTION(msg);
    }
    auto* scope = static_cast<AudioWorkletGlobalScope*>(
        state->context()->globalObject()->extraData());
    Optional<MessagePort*> port = scope->takePendingPort();
    if (!port) {
        THROW_EXCEPTION(
            "AudioWorkletProcessor is constructed only for an "
            "AudioWorkletNode");
    }
    auto* processor =
        new AudioWorkletProcessor(scope->executionContext(), port.value());
    ScriptBindingInstance* instance = scope->scriptBindingInstance();
    if (newTarget.value() != instance->fnAudioWorkletProcessor()) {
        ValueRef* proto =
            newTarget->isFunctionObject()
                ? newTarget->asFunctionObject()->getFunctionPrototype(state)
                : newTarget->get(state, scriptStringPrototype(instance));
        processor->scriptObject()->setPrototype(state, proto);
    }
    return processor->scriptValue();
}
} // namespace Starfish

#endif
