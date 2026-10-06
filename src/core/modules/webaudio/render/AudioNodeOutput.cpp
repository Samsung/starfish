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
#include "core/modules/webaudio/render/AudioNodeOutput.h"
#include "core/modules/webaudio/render/AudioGraph.h"

#include <algorithm>

namespace Starfish {

AudioNodeOutput::AudioNodeOutput(AudioHandler* owner, size_t channels)
    : m_owner(owner)
    , m_bus(channels)
{
}

void AudioNodeOutput::configureChannels(size_t channels)
{
    if (channels == m_bus.channels()) {
        return;
    }
    m_bus.resize(channels);
    for (AudioNodeInput* destination : m_destinations) {
        destination->refreshChannels();
    }
}

void AudioNodeOutput::attachInput(AudioNodeInput* input)
{
    m_destinations.push_back(input);
}

void AudioNodeOutput::detachInput(AudioNodeInput* input)
{
    auto it = std::find(m_destinations.begin(), m_destinations.end(), input);
    if (it != m_destinations.end()) {
        m_destinations.erase(it);
    }
}

} // namespace Starfish

#endif
