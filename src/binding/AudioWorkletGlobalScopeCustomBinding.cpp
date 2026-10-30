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
#include "core/dom/DOMException.h"
#include "core/modules/webaudio/AudioWorkletGlobalScope.h"

#include <EscargotPublic.h>

#include <cmath>
#include <cstring>

using namespace Escargot;

namespace Starfish {

extern AudioParamDescriptor toAudioParamDescriptorFromValueRef(
    ExecutionStateRef* state, ValueRef* from);

static void throwTypeError(ExecutionStateRef* state, const char* message)
{
    state->throwException(TypeErrorObjectRef::create(
        state, StringRef::createFromASCII(message, strlen(message))));
}

static void throwDOMException(ExecutionStateRef* state,
                              AudioWorkletGlobalScope* scope,
                              DOMException::Code code, const char* message)
{
    state->throwException(
        (new DOMException(scope->executionContext(), code, message))
            ->scriptValue());
}

// WebIDL sequence<AudioParamDescriptor> conversion; a float member that is
// not finite fails the float conversion.
static void readParameterDescriptors(ExecutionStateRef* state, ValueRef* value,
                                     GCVector<AudioParamDescriptor>& result)
{
    if (!value->isObject()) {
        throwTypeError(state, "parameterDescriptors is not iterable");
    }
    ValueRef* method = value->asObject()->get(
        state, state->context()->vmInstance()->iteratorSymbol());
    if (!method->isCallable()) {
        throwTypeError(state, "parameterDescriptors is not iterable");
    }
    ValueRef* iterator = method->call(state, value, 0, nullptr);
    if (!iterator->isObject()) {
        throwTypeError(state, "Iterator is not an object");
    }
    ValueRef* next =
        iterator->asObject()->get(state, StringRef::createFromASCII("next"));
    while (true) {
        ValueRef* step = next->call(state, iterator, 0, nullptr);
        if (!step->isObject()) {
            throwTypeError(state, "Iterator result is not an object");
        }
        if (step->asObject()
                ->get(state, StringRef::createFromASCII("done"))
                ->toBoolean(state)) {
            break;
        }
        AudioParamDescriptor descriptor = toAudioParamDescriptorFromValueRef(
            state,
            step->asObject()->get(state, StringRef::createFromASCII("value")));
        if (!std::isfinite(descriptor.defaultValue()) ||
            !std::isfinite(descriptor.minValue()) ||
            !std::isfinite(descriptor.maxValue())) {
            throwTypeError(state, "AudioParamDescriptor value is not finite");
        }
        result.push_back(descriptor);
    }
}

// https://webaudio.github.io/web-audio-api/#dom-audioworkletglobalscope-registerprocessor
ValueRef* registerProcessorAudioWorkletGlobalScopeFunction(
    ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv,
    bool isNewExpression)
{
    // An operation of a [Global] interface called with an undefined or null
    // this value runs on the realm's global object (WebIDL).
    GENERATE_WORKER_GLOBALSCOPE(AudioWorkletGlobalScope);
    if (argc < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argc);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "registerProcessor",
                        "AudioWorkletGlobalScope", reason);
        THROW_EXCEPTION(msg);
    }
    String* name = toBrowserString(state, argv[0]);
    ValueRef* processorCtor = argv[1];
    // Converting the argument to the AudioWorkletProcessorConstructor
    // callback type precedes the method steps.
    if (!processorCtor->isCallable()) {
        throwTypeError(state, "processorCtor is not a function");
    }
    if (!name->length() || originalObj->hasProcessor(name)) {
        throwDOMException(state, originalObj, DOMException::NOT_SUPPORTED_ERR,
                          "Invalid or already registered processor name");
    }
    if (!processorCtor->isConstructible()) {
        throwTypeError(state, "processorCtor is not a constructor");
    }
    ObjectRef* ctor = processorCtor->asObject();
    if (!ctor->get(state, StringRef::createFromASCII("prototype"))
             ->isObject()) {
        throwTypeError(state, "processorCtor.prototype is not an object");
    }
    auto* definition = new AudioWorkletProcessorDefinition;
    definition->name = name;
    ValueRef* descriptors =
        ctor->get(state, StringRef::createFromASCII("parameterDescriptors"));
    if (!descriptors->isUndefined()) {
        readParameterDescriptors(state, descriptors, definition->descriptors);
        for (size_t i = 0; i < definition->descriptors.size(); i++) {
            const AudioParamDescriptor& descriptor = definition->descriptors[i];
            for (size_t j = 0; j < i; j++) {
                if (definition->descriptors[j].name()->equals(
                        descriptor.name())) {
                    throwDOMException(state, originalObj,
                                      DOMException::NOT_SUPPORTED_ERR,
                                      "Duplicate AudioParamDescriptor name");
                }
            }
            if (!(descriptor.minValue() <= descriptor.defaultValue() &&
                  descriptor.defaultValue() <= descriptor.maxValue())) {
                throwDOMException(state, originalObj,
                                  DOMException::INVALID_STATE_ERR,
                                  "AudioParamDescriptor default value is "
                                  "outside its range");
            }
        }
    }
    originalObj->addProcessor(definition, processorCtor);
    return ValueRef::createUndefined();
}
} // namespace Starfish

#endif
