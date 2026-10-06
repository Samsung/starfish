/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioNodeInput__
#define __StarfishAudioNodeInput__

#include "core/modules/webaudio/render/AudioBus.h"

#include <cstdint>
#include <vector>

namespace Starfish {

class AudioHandler;
class AudioNodeOutput;

enum class AudioInputChannelMode { Max, ClampedMax, Explicit };

class AudioNodeInput {
public:
    AudioNodeInput(AudioHandler* owner, size_t index, size_t channelCount = 2);

    bool connect(AudioNodeOutput* output);
    bool disconnect(AudioNodeOutput* output);
    void disconnectAll();
    void configure(size_t channelCount, AudioInputChannelMode mode,
                   bool speakers);
    void refreshChannels();
    // Mixes the sources' outputs, which the graph rendered earlier this
    // quantum.
    const AudioBus& pull(size_t frames);
    void clearForTeardown()
    {
        m_sources.clear();
    }
    const std::vector<AudioNodeOutput*>& sources() const
    {
        return m_sources;
    }
    const AudioBus& bus() const
    {
        return m_bus;
    }
    size_t connectionCount() const
    {
        return m_sources.size();
    }
    AudioHandler* owner() const
    {
        return m_owner;
    }

private:
    AudioHandler* m_owner;
    size_t m_index;
    std::vector<AudioNodeOutput*> m_sources;
    AudioBus m_bus;
    size_t m_channelCount{ 2 };
    AudioInputChannelMode m_mode{ AudioInputChannelMode::Max };
    bool m_speakers{ true };
};

} // namespace Starfish

#endif
#endif
