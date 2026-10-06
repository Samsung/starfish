/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishOfflineAudioCompletionEvent__
#define __StarfishOfflineAudioCompletionEvent__

#include "core/dom/Event.h"

namespace Starfish {
class AudioBuffer;

struct OfflineAudioCompletionEventInit : public EventInit {
    DEFINE_GETTER_SETTER(AudioBuffer*, renderedBuffer, RenderedBuffer)
    AudioBuffer* m_renderedBuffer{ nullptr };
};

class OfflineAudioCompletionEvent : public Event {
public:
    OfflineAudioCompletionEvent(ExecutionContext* executionContext)
        : Event(executionContext)
    {
    }

    OfflineAudioCompletionEvent(ExecutionContext* executionContext,
                                String* type,
                                OfflineAudioCompletionEventInit init)
        : Event(executionContext, type)
        , m_renderedBuffer(init.m_renderedBuffer)
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(OfflineAudioCompletionEvent)
    DEFINE_GETTER(AudioBuffer*, renderedBuffer)

private:
    AudioBuffer* m_renderedBuffer{ nullptr };
};
} // namespace Starfish

#endif
#endif
