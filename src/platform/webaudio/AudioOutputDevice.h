/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioOutputDevice__
#define __StarfishAudioOutputDevice__

#include <cstdint>
#include <memory>

namespace Starfish {

class AudioBus;

// A native-only sink. A null return from create() means the context keeps
// rendering against its wall clock without a hardware output device.
class AudioOutputDevice {
public:
    virtual ~AudioOutputDevice() = default;

    static std::unique_ptr<AudioOutputDevice> create(uint32_t sampleRate);
    virtual void submit(const AudioBus& bus, uint64_t firstFrame) = 0;
    virtual bool failed() const = 0;
    virtual double outputLatency() const
    {
        return 0;
    }
    virtual bool outputTimestamp(double& contextTime,
                                 uint64_t& wallClockUs) const
    {
        return false;
    }
};

} // namespace Starfish

#endif
#endif
