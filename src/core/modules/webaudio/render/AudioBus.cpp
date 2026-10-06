/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioBus.h"

#include <algorithm>

namespace Starfish {

const size_t AudioBus::RenderQuantumFrames;

static const float kSqrtHalf = 0.7071067811865476f;

AudioBus::AudioBus(size_t channels, size_t frames)
{
    resize(channels, frames);
}

void AudioBus::resize(size_t channels, size_t frames)
{
    STARFISH_ASSERT(channels <= 32);
    STARFISH_ASSERT(frames <= RenderQuantumFrames);
    if (m_channels == channels && m_frames == frames) {
        return;
    }

    if (channels * frames > m_capacity) {
        m_data.reset(new float[channels * frames]);
        m_capacity = channels * frames;
    }
    m_channels = channels;
    m_frames = frames;
    zero();
}

void AudioBus::zero()
{
    if (m_data) {
        std::fill(m_data.get(), m_data.get() + m_channels * m_frames, 0.0f);
    }
    m_isSilent = true;
}

float* AudioBus::channel(size_t index)
{
    STARFISH_ASSERT(index < m_channels);
    return m_data.get() + index * m_frames;
}

const float* AudioBus::channel(size_t index) const
{
    STARFISH_ASSERT(index < m_channels);
    return m_data.get() + index * m_frames;
}

void AudioBus::copyFrom(const AudioBus& source, size_t frames, bool speakers)
{
    zero();
    addFrom(source, frames, speakers);
}

// https://webaudio.github.io/web-audio-api/#channel-up-mixing-and-down-mixing
void AudioBus::addFrom(const AudioBus& source, size_t frames, bool speakers)
{
    STARFISH_ASSERT(frames <= m_frames && frames <= source.m_frames);
    if (source.m_isSilent || !source.m_channels || !m_channels) {
        return;
    }

    const size_t in = source.m_channels;
    const size_t out = m_channels;
    m_isSilent = false;

    if (speakers && in == 1 && (out == 2 || out == 4)) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] += source.channel(0)[i];
            channel(1)[i] += source.channel(0)[i];
        }
        return;
    }
    if (speakers && in == 1 && out == 6) {
        for (size_t i = 0; i < frames; i++) {
            channel(2)[i] += source.channel(0)[i];
        }
        return;
    }
    if (speakers && in == 4 && out == 6) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] += source.channel(0)[i];
            channel(1)[i] += source.channel(1)[i];
            channel(4)[i] += source.channel(2)[i];
            channel(5)[i] += source.channel(3)[i];
        }
        return;
    }
    if (speakers && in == 2 && out == 1) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] +=
                0.5f * (source.channel(0)[i] + source.channel(1)[i]);
        }
        return;
    }
    if (speakers && in == 4 && out == 1) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] +=
                0.25f * (source.channel(0)[i] + source.channel(1)[i] +
                         source.channel(2)[i] + source.channel(3)[i]);
        }
        return;
    }
    if (speakers && in == 6 && out == 1) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] +=
                kSqrtHalf * (source.channel(0)[i] + source.channel(1)[i]) +
                source.channel(2)[i] +
                0.5f * (source.channel(4)[i] + source.channel(5)[i]);
        }
        return;
    }
    if (speakers && in == 4 && out == 2) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] +=
                0.5f * (source.channel(0)[i] + source.channel(2)[i]);
            channel(1)[i] +=
                0.5f * (source.channel(1)[i] + source.channel(3)[i]);
        }
        return;
    }
    if (speakers && in == 6 && out == 2) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] +=
                source.channel(0)[i] +
                kSqrtHalf * (source.channel(2)[i] + source.channel(4)[i]);
            channel(1)[i] +=
                source.channel(1)[i] +
                kSqrtHalf * (source.channel(2)[i] + source.channel(5)[i]);
        }
        return;
    }
    if (speakers && in == 6 && out == 4) {
        for (size_t i = 0; i < frames; i++) {
            channel(0)[i] +=
                source.channel(0)[i] + kSqrtHalf * source.channel(2)[i];
            channel(1)[i] +=
                source.channel(1)[i] + kSqrtHalf * source.channel(2)[i];
            channel(2)[i] += source.channel(4)[i];
            channel(3)[i] += source.channel(5)[i];
        }
        return;
    }

    // Same-layout, stereo up-mix, and unsupported speaker layouts use the
    // discrete rule: copy corresponding channels and leave extras silent.
    const size_t common = std::min(in, out);
    for (size_t c = 0; c < common; c++) {
        const float* input = source.channel(c);
        float* output = channel(c);
        for (size_t i = 0; i < frames; i++) {
            output[i] += input[i];
        }
    }
}

} // namespace Starfish

#endif
