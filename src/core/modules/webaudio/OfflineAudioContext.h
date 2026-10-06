/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishOfflineAudioContext__
#define __StarfishOfflineAudioContext__

#include "core/modules/webaudio/BaseAudioContext.h"

namespace Starfish {
class AudioBuffer;

struct OfflineAudioContextOptions {
    DEFINE_GETTER_SETTER(uint32_t, numberOfChannels, NumberOfChannels)
    DEFINE_GETTER_SETTER(uint32_t, length, Length)
    DEFINE_GETTER(double, sampleRate)

    void setSampleRate(double value)
    {
        m_sampleRate = value;
        m_hasSampleRate = true;
    }
    bool hasSampleRate() const
    {
        return m_hasSampleRate;
    }

    uint32_t m_numberOfChannels{ 1 };
    uint32_t m_length{ 0 };
    double m_sampleRate{ 0 };
    bool m_hasSampleRate{ false };
};

class OfflineAudioContext : public BaseAudioContext {
public:
    OfflineAudioContext(ExecutionContext* executionContext,
                        OfflineAudioContextOptions options);
    OfflineAudioContext(ExecutionContext* executionContext,
                        uint32_t numberOfChannels, uint32_t length,
                        double sampleRate);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(OfflineAudioContext)
    DEFINE_GETTER(uint32_t, length)

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(complete);
#undef VIRTUAL
#undef OVERRIDE

    Promise* startRendering();
    Promise* suspend(double suspendTime);
    Promise* resume();

private:
    struct PendingSuspend {
        uint64_t frame;
        Promise* promise;
    };

    void renderChunk();
    void queueRenderChunk();

    uint32_t m_length;
    uint32_t m_numberOfChannels;
    uint32_t m_renderedFrames{ 0 };
    AudioBuffer* m_renderedBuffer{ nullptr };
    Promise* m_renderPromise{ nullptr };
    GCVector<PendingSuspend> m_pendingSuspends;
    bool m_started{ false };
};
} // namespace Starfish

#endif
#endif
