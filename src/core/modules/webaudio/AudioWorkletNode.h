/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishAudioWorkletNode__
#define __StarfishAudioWorkletNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParamMap;
class MessagePort;

// https://webaudio.github.io/web-audio-api/#dictdef-audioworkletnodeoptions
struct AudioWorkletNodeOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(uint32_t, numberOfInputs, NumberOfInputs)
    DEFINE_GETTER_SETTER(uint32_t, numberOfOutputs, NumberOfOutputs)
    const GCAtomicVector<uint32_t>& outputChannelCount() const
    {
        return m_outputChannelCount;
    }
    void setOutputChannelCount(const GCAtomicVector<uint32_t>& value)
    {
        m_outputChannelCount = value;
        m_hasOutputChannelCount = true;
    }
    bool hasOutputChannelCount() const
    {
        return m_hasOutputChannelCount;
    }
    // Null when absent.
    DEFINE_GETTER_SETTER(ScriptObject, parameterData, ParameterData)
    DEFINE_GETTER_SETTER(ScriptObject, processorOptions, ProcessorOptions)

    uint32_t m_numberOfInputs{ 1 };
    uint32_t m_numberOfOutputs{ 1 };
    GCAtomicVector<uint32_t> m_outputChannelCount;
    bool m_hasOutputChannelCount{ false };
    ScriptObject m_parameterData{ nullptr };
    ScriptObject m_processorOptions{ nullptr };
};

// https://webaudio.github.io/web-audio-api/#AudioWorkletNode
class AudioWorkletNode : public AudioNode {
public:
    AudioWorkletNode(
        ExecutionContext* executionContext, BaseAudioContext* context,
        String* name,
        AudioWorkletNodeOptions options = AudioWorkletNodeOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioWorkletNode)

    AudioParamMap* parameters() const
    {
        return m_parameters;
    }
    MessagePort* port() const
    {
        return m_port;
    }

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(processorerror);
#undef VIRTUAL
#undef OVERRIDE

private:
    AudioParamMap* m_parameters{ nullptr };
    MessagePort* m_port{ nullptr };
};
} // namespace Starfish

#endif
#endif
