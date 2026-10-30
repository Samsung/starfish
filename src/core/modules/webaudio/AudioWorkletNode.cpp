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
#include "Starfish.h"

#include "core/modules/webaudio/AudioWorkletNode.h"

#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/AudioParamMap.h"
#include "core/modules/webaudio/AudioWorklet.h"
#include "core/modules/webaudio/AudioWorkletGlobalScope.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioWorkletHandler.h"
#include "core/serialize/Serializer.h"

#include <EscargotPublic.h>

#include <cmath>
#include <cstring>
#include <new>

namespace Starfish {

using namespace Escargot;

typedef GCVector<std::pair<String*, double>> ParameterValues;

// WebIDL conversion of parameterData to record<DOMString, double>: own
// enumerable string keys in property order, values as restricted doubles.
static ParameterValues parameterValues(ScriptBindingInstance* instance,
                                       ScriptObject data)
{
    ParameterValues result;
    auto evaluated = Evaluator::execute(
        instance->scriptContext(),
        [](ExecutionStateRef* state, ObjectRef* data,
           ParameterValues* result) -> ValueRef* {
            ValueVectorRef* keys = data->ownPropertyKeys(state);
            for (size_t i = 0; i < keys->size(); i++) {
                ValueRef* key = keys->at(i);
                if (!key->isString()) {
                    continue;
                }
                ValueRef* descriptor =
                    data->getOwnPropertyDescriptor(state, key);
                if (descriptor->isUndefined() ||
                    !descriptor->asObject()
                         ->get(state, StringRef::createFromASCII("enumerable"))
                         ->toBoolean(state)) {
                    continue;
                }
                const double value = data->get(state, key)->toNumber(state);
                if (!std::isfinite(value)) {
                    const char* message = "parameterData value is not finite";
                    state->throwException(TypeErrorObjectRef::create(
                        state,
                        StringRef::createFromASCII(message, strlen(message))));
                }
                result->push_back(
                    std::make_pair(toBrowserString(state, key), value));
            }
            return ValueRef::createUndefined();
        },
        data, &result);
    if (evaluated.error.hasValue()) {
        throwScriptException(instance, evaluated.error.value());
    }
    return result;
}

struct OptionsObjectInput {
    const AudioWorkletNodeOptions* options;
    const ParameterValues* parameterData;
};

// https://webaudio.github.io/web-audio-api/#dom-audioworkletnode-audioworkletnode
// "Convert options dictionary to optionsObject": the members that are
// present, inherited ones first, then in lexicographic order.
static ScriptValue optionsObject(ScriptBindingInstance* instance,
                                 const OptionsObjectInput& input)
{
    return Evaluator::execute(
               instance->scriptContext(),
               [](ExecutionStateRef* state,
                  const OptionsObjectInput* input) -> ValueRef* {
                   const AudioWorkletNodeOptions& options = *input->options;
                   ObjectRef* result = ObjectRef::create(state);
                   auto set = [&](const char* name, ValueRef* value) {
                       result->set(
                           state,
                           StringRef::createFromASCII(name, strlen(name)),
                           value);
                   };
                   if (options.hasChannelCount()) {
                       set("channelCount",
                           ValueRef::create(options.channelCount()));
                   }
                   if (options.hasChannelCountMode()) {
                       set("channelCountMode",
                           toJSString(options.channelCountMode()));
                   }
                   if (options.hasChannelInterpretation()) {
                       set("channelInterpretation",
                           toJSString(options.channelInterpretation()));
                   }
                   set("numberOfInputs",
                       ValueRef::create(options.numberOfInputs()));
                   set("numberOfOutputs",
                       ValueRef::create(options.numberOfOutputs()));
                   if (options.hasOutputChannelCount()) {
                       ArrayObjectRef* counts = ArrayObjectRef::create(state);
                       const auto& values = options.outputChannelCount();
                       for (size_t i = 0; i < values.size(); i++) {
                           counts->set(state, ValueRef::create(i),
                                       ValueRef::create(values[i]));
                       }
                       set("outputChannelCount", counts);
                   }
                   if (options.parameterData()) {
                       ObjectRef* data = ObjectRef::create(state);
                       for (const auto& entry : *input->parameterData) {
                           data->set(state, toJSString(entry.first),
                                     ValueRef::create(entry.second));
                       }
                       set("parameterData", data);
                   }
                   if (options.processorOptions()) {
                       set("processorOptions", options.processorOptions());
                   }
                   return result;
               },
               &input)
        .result;
}

AudioWorkletNode::AudioWorkletNode(ExecutionContext* executionContext,
                                   BaseAudioContext* context, String* name,
                                   AudioWorkletNodeOptions options)
    : AudioNode(executionContext, context)
{
    // parameterData is a WebIDL record, converted with the other members
    // before the constructor steps run.
    ParameterValues parameterData;
    if (options.parameterData()) {
        parameterData =
            parameterValues(scriptBindingInstance(), options.parameterData());
    }
    // https://webaudio.github.io/web-audio-api/#dom-audioworkletnode-audioworkletnode
    AudioWorklet* worklet = context->audioWorklet();
    Optional<AudioWorkletProcessorDefinition*> definition =
        worklet->definition(name);
    if (!definition) {
        throw new DOMException(executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "No AudioWorkletProcessor of this name");
    }
    // Initializing the AudioNode applies channelCount before the channel
    // configuration below is checked.
    if (options.hasChannelCount() &&
        (options.channelCount() < 1 || options.channelCount() > 32)) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported AudioNode channel count");
    }
    // https://webaudio.github.io/web-audio-api/#configuring-channels-with-audioworkletnodeoptions
    const uint32_t inputs = options.numberOfInputs();
    const uint32_t outputs = options.numberOfOutputs();
    // The ED sets no maximum. Every input and output holds buffers sized
    // by the render quantum, so Starfish allows as many as a
    // ChannelMergerNode's inputs.
    if ((!inputs && !outputs) || inputs > 32 || outputs > 32) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported number of inputs or outputs");
    }
    std::vector<size_t> outputChannels(outputs, 1);
    if (options.hasOutputChannelCount()) {
        const auto& counts = options.outputChannelCount();
        for (uint32_t count : counts) {
            if (!count || count > 32) {
                throw new DOMException(executionContext,
                                       DOMException::NOT_SUPPORTED_ERR,
                                       "Unsupported output channel count");
            }
        }
        if (counts.size() != outputs) {
            throw new DOMException(executionContext,
                                   DOMException::INDEX_SIZE_ERR,
                                   "outputChannelCount length differs from "
                                   "numberOfOutputs");
        }
        for (size_t index = 0; index < outputs; index++) {
            outputChannels[index] = counts[index];
        }
    }

    AudioGraphLock graphLock(context->graph());
    m_numberOfInputs = inputs;
    m_numberOfOutputs = outputs;
    const auto& descriptors = definition.value()->descriptors;
    std::unique_ptr<AudioWorkletHandler> handler;
    try {
        handler.reset(new AudioWorkletHandler(inputs, outputChannels));
        for (const AudioParamDescriptor& descriptor : descriptors) {
            handler->addTimeline(descriptor.defaultValue(),
                                 descriptor.minValue(), descriptor.maxValue(),
                                 descriptor.automationRate()->equals("a-rate"));
        }
    } catch (const std::bad_alloc&) {
        context->throwAllocationError();
    }
    AudioWorkletHandler* workletHandler = handler.get();
    m_handler = context->addHandler(std::move(handler));
    applyOptions(options);

    // The node's port and the processor's port are one MessageChannel.
    // Serializing the processor's end with transfer and deserializing it in
    // the global scope's realm, both on this thread, is observably the same
    // as entangling it with a port created in that realm.
    AudioWorkletGlobalScope* globalScope = worklet->globalScope();
    m_port = new MessagePort(executionContext);
    MessagePort* processorPort =
        new MessagePort(globalScope->executionContext());
    MessagePort::entangle(m_port, processorPort);
    // StructuredSerialize(optionsObject); DataCloneError propagates.
    auto* serializedOptions = new (GC) SerializeWithTransferResult();
    Serializer::serializeWithTransfer(
        executionContext,
        optionsObject(scriptBindingInstance(), { &options, &parameterData }),
        GCVector<ScriptObject>(), *serializedOptions);

    m_parameters = new AudioParamMap(executionContext);
    for (size_t index = 0; index < descriptors.size(); index++) {
        AudioParamTimeline* timeline = workletHandler->timeline(index);
        context->graph()->registerTimeline(timeline);
        m_parameters->set(descriptors[index].name(),
                          new AudioParam(context, timeline));
    }
    for (const auto& entry : parameterData) {
        if (Optional<AudioParam*> param = m_parameters->get(entry.first)) {
            param.value()->setValue(static_cast<float>(entry.second));
        }
    }

    workletHandler->setRecord(globalScope->createProcessor(
        this, definition.value(), processorPort, serializedOptions));
}

ScriptBindingInstance* AudioWorkletNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

DEFINE_EVENT_LISTENER(AudioWorkletNode, processorerror);
} // namespace Starfish

#endif
