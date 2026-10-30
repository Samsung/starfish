/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishAudioWorkletGlobalScope__
#define __StarfishAudioWorkletGlobalScope__

#include "core/modules/worklet/WorkletGlobalScope.h"

namespace Starfish {
class AudioWorklet;
class AudioWorkletNode;
class BaseAudioContext;
class MessagePort;
class ResourceURL;
class SerializeWithTransferResult;

// https://webaudio.github.io/web-audio-api/#AudioParamDescriptor
struct AudioParamDescriptor {
    DEFINE_GETTER_SETTER(String*, name, Name)
    DEFINE_GETTER_SETTER(float, defaultValue, DefaultValue)
    DEFINE_GETTER_SETTER(float, minValue, MinValue)
    DEFINE_GETTER_SETTER(float, maxValue, MaxValue)
    DEFINE_GETTER_SETTER(String*, automationRate, AutomationRate)
    String* m_name{ nullptr };
    float m_defaultValue{ 0 };
    float m_minValue{ -3.4028235e38f };
    float m_maxValue{ 3.4028235e38f };
    String* m_automationRate{ nullptr };
};

// One registerProcessor() call: the name and its parameter descriptors.
struct AudioWorkletProcessorDefinition : public gc {
    String* name;
    GCVector<AudioParamDescriptor> descriptors;
};

// The global scope's side of one AudioWorkletNode. The scope keeps it, and
// with it the processor, until the graph deletes the node's handler, which
// can outlive the node's wrapper.
class AudioWorkletProcessorRecord : public gc {
public:
    AudioWorkletProcessorRecord(AudioWorkletNode* node,
                                AudioWorkletProcessorDefinition* definition,
                                MessagePort* port,
                                SerializeWithTransferResult* options);

    // The node unless it was collected: a strong reference would keep it,
    // and so its handler and this record, alive for good.
    Optional<AudioWorkletNode*> node() const;

    AudioWorkletProcessorDefinition* const definition;
    // The processor's end of the node's MessageChannel.
    MessagePort* const port;
    // The serialized AudioWorkletNodeOptions until the processor is built.
    SerializeWithTransferResult* options;
    Optional<ScriptObject> processor;

private:
    // GC_MALLOC_ATOMIC cell holding the hidden node pointer; a disappearing
    // link clears it when the node is collected.
    void** m_nodeLink;
};

// https://webaudio.github.io/web-audio-api/#AudioWorkletGlobalScope
// The ED runs AudioWorkletGlobalScope on the rendering thread. Starfish keeps
// one realm per context on the main thread so worklets need no
// thread-isolated GC (WORKER=1); a context holding AudioWorkletNodes is
// rendered by the main thread.
class AudioWorkletGlobalScope : public WorkletGlobalScope {
public:
    AudioWorkletGlobalScope(BaseAudioContext* context, AudioWorklet* worklet);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioWorkletGlobalScope)

    ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    bool hasProcessor(String* name) const;
    // The checks of registerProcessor() run in its custom binding, which
    // calls this last.
    void addProcessor(AudioWorkletProcessorDefinition* definition,
                      ScriptValue processorCtor);

    uint64_t currentFrame() const;
    double currentTime() const;
    float sampleRate() const;
    uint32_t renderQuantumSize() const;
    MessagePort* port() const
    {
        return m_port;
    }

    // Parses and runs a fetched module script. Returns false and sets
    // |error| when the script cannot be created; evaluation errors are
    // reported, as "run a module script" does.
    bool runModuleScript(String* source, ResourceURL* url, ScriptValue& error);

    // Called by the AudioWorkletNode constructor: records the node and
    // queues the construction of its processor.
    AudioWorkletProcessorRecord* createProcessor(
        AudioWorkletNode* node, AudioWorkletProcessorDefinition* definition,
        MessagePort* port, SerializeWithTransferResult* options);
    // https://webaudio.github.io/web-audio-api/#pending-processor-construction-data
    // The AudioWorkletProcessor constructor takes the transferred port; the
    // slot is then empty.
    Optional<MessagePort*> takePendingPort();

private:
    void dispose();
    void constructProcessor(AudioWorkletProcessorRecord* record);
    void queueProcessorError(AudioWorkletProcessorRecord* record,
                             String* message, String* filename, uint32_t lineno,
                             uint32_t colno);
    // Drops the records of handlers the graph has deleted.
    void sweepRecords();

    BaseAudioContext* m_context;
    AudioWorklet* m_worklet;
    ScriptBindingInstance* m_scriptBindingInstance;
    ExecutionContext* m_executionContext;
    MessagePort* m_port;
    // node name to processor constructor map
    GCVector<std::pair<String*, ScriptValue>> m_processorConstructors;
    GCVector<AudioWorkletProcessorRecord*> m_records;
    Optional<MessagePort*> m_pendingPort;
};
} // namespace Starfish

#endif
#endif
