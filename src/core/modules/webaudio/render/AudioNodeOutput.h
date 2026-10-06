/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioNodeOutput__
#define __StarfishAudioNodeOutput__

#include "core/modules/webaudio/render/AudioBus.h"

#include <cstdint>
#include <vector>

namespace Starfish {

class AudioHandler;
class AudioNodeInput;

class AudioNodeOutput {
public:
    AudioNodeOutput(AudioHandler* owner, size_t channels);

    AudioBus& bus()
    {
        return m_bus;
    }
    const AudioBus& bus() const
    {
        return m_bus;
    }
    void clearForTeardown()
    {
        m_destinations.clear();
    }
    size_t channels() const
    {
        return m_bus.channels();
    }
    void configureChannels(size_t channels);
    void attachInput(AudioNodeInput* input);
    void detachInput(AudioNodeInput* input);
    AudioHandler* owner() const
    {
        return m_owner;
    }
    const std::vector<AudioNodeInput*>& destinations() const
    {
        return m_destinations;
    }

private:
    AudioHandler* m_owner;
    AudioBus m_bus;
    std::vector<AudioNodeInput*> m_destinations;
};

} // namespace Starfish

#endif
#endif
