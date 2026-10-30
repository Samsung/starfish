/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishScriptProcessorNode__
#define __StarfishScriptProcessorNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioBuffer;
class ScriptProcessorHandler;

class ScriptProcessorNode : public AudioNode {
public:
    ScriptProcessorNode(ExecutionContext* executionContext,
                        BaseAudioContext* context, uint32_t bufferSize,
                        uint32_t numberOfInputChannels,
                        uint32_t numberOfOutputChannels);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ScriptProcessorNode)

    EventListener* onaudioprocess();
    void setOnaudioprocess(EventListener* listener);
    int32_t bufferSize() const
    {
        return static_cast<int32_t>(m_bufferSize);
    }
    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* channelCountMode) override;

    // Main thread: fires audioprocess for every input block the render side
    // has completed. False once the node has neither an audioprocess
    // listener nor a connection, so the context stops keeping it alive.
    bool dispatchAudioProcessEvents();

private:
    ScriptProcessorHandler* m_processorHandler{ nullptr };
    // Allocated on the first event and reused: one inputBuffer, and one
    // outputBuffer per handler slot, since an output plays while the next
    // event runs.
    Optional<AudioBuffer*> m_inputBuffer;
    Optional<AudioBuffer*> m_outputBuffers[2];
    uint32_t m_bufferSize;
    uint32_t m_numberOfInputChannels;
    uint32_t m_numberOfOutputChannels;
};
} // namespace Starfish

#endif
#endif
