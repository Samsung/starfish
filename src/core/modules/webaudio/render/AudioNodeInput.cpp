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
#include "core/modules/webaudio/render/AudioNodeInput.h"
#include "core/modules/webaudio/render/AudioGraph.h"

#include <algorithm>

namespace Starfish {

AudioNodeInput::AudioNodeInput(AudioHandler* owner, size_t index,
                               size_t channelCount)
    : m_owner(owner)
    , m_index(index)
    , m_bus(channelCount)
    , m_channelCount(channelCount)
{
}

bool AudioNodeInput::connect(AudioNodeOutput* output)
{
    if (std::find(m_sources.begin(), m_sources.end(), output) !=
        m_sources.end()) {
        return false;
    }
    m_sources.push_back(output);
    output->attachInput(this);
    refreshChannels();
    m_owner->graph()->markTopologyDirty();
    return true;
}

bool AudioNodeInput::disconnect(AudioNodeOutput* output)
{
    auto it = std::find(m_sources.begin(), m_sources.end(), output);
    if (it == m_sources.end()) {
        return false;
    }
    m_sources.erase(it);
    output->detachInput(this);
    refreshChannels();
    m_owner->graph()->markTopologyDirty();
    return true;
}

void AudioNodeInput::disconnectAll()
{
    for (AudioNodeOutput* source : m_sources) {
        source->detachInput(this);
    }
    m_sources.clear();
    refreshChannels();
    m_owner->graph()->markTopologyDirty();
}

void AudioNodeInput::configure(size_t channelCount, AudioInputChannelMode mode,
                               bool speakers)
{
    STARFISH_ASSERT(channelCount >= 1 && channelCount <= 32);
    m_channelCount = channelCount;
    m_mode = mode;
    m_speakers = speakers;
    refreshChannels();
}

// https://webaudio.github.io/web-audio-api/#computednumberofchannels
void AudioNodeInput::refreshChannels()
{
    size_t channels = 1;
    for (AudioNodeOutput* source : m_sources) {
        channels = std::max(channels, source->channels());
    }
    if (m_mode == AudioInputChannelMode::Explicit) {
        channels = m_channelCount;
    } else if (m_mode == AudioInputChannelMode::ClampedMax) {
        channels = std::min(channels, m_channelCount);
    }
    if (m_bus.channels() != channels) {
        m_bus.resize(channels);
        m_owner->inputChannelsChanged(m_index);
    }
}

const AudioBus& AudioNodeInput::pull(size_t frames)
{
    m_bus.zero();
    for (AudioNodeOutput* source : m_sources) {
        m_bus.addFrom(source->bus(), frames, m_speakers);
    }
    return m_bus;
}

} // namespace Starfish

#endif
