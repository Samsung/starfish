/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishAudioWorkletProcessor__
#define __StarfishAudioWorkletProcessor__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class ExecutionContext;
class MessagePort;

// https://webaudio.github.io/web-audio-api/#AudioWorkletProcessor
class AudioWorkletProcessor : public ScriptWrappable {
public:
    AudioWorkletProcessor(ExecutionContext* executionContext, MessagePort* port)
        : ScriptWrappable(this)
        , m_executionContext(executionContext)
        , m_port(port)
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioWorkletProcessor)

    MessagePort* port() const
    {
        return m_port;
    }

private:
    ExecutionContext* m_executionContext;
    MessagePort* m_port;
};
} // namespace Starfish

#endif
#endif
