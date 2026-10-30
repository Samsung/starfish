/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioProcessingEvent__
#define __StarfishAudioProcessingEvent__

#include "core/dom/Event.h"

namespace Starfish {
class AudioBuffer;

struct AudioProcessingEventInit : public EventInit {
    DEFINE_GETTER_SETTER(double, playbackTime, PlaybackTime)
    DEFINE_GETTER_SETTER(AudioBuffer*, inputBuffer, InputBuffer)
    DEFINE_GETTER_SETTER(AudioBuffer*, outputBuffer, OutputBuffer)
    double m_playbackTime{ 0 };
    AudioBuffer* m_inputBuffer{ nullptr };
    AudioBuffer* m_outputBuffer{ nullptr };
};

class AudioProcessingEvent : public Event {
public:
    AudioProcessingEvent(ExecutionContext* executionContext, String* type,
                         AudioProcessingEventInit init)
        : Event(executionContext, type, init)
        , m_playbackTime(init.m_playbackTime)
        , m_inputBuffer(init.m_inputBuffer)
        , m_outputBuffer(init.m_outputBuffer)
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioProcessingEvent)
    DEFINE_GETTER(double, playbackTime)
    DEFINE_GETTER(AudioBuffer*, inputBuffer)
    DEFINE_GETTER(AudioBuffer*, outputBuffer)

private:
    double m_playbackTime;
    AudioBuffer* m_inputBuffer;
    AudioBuffer* m_outputBuffer;
};
} // namespace Starfish

#endif
#endif
