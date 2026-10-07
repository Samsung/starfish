/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#ifndef __StarfishFFmpegAudioOutput__
#define __StarfishFFmpegAudioOutput__

#include <cstddef>
#include <cstdint>

namespace Starfish {

// Open, write, flush and destruction belong to the audio writer thread.
// interrupt() may be called by the owner to cancel a pending device write.
class FFmpegAudioOutput {
public:
    virtual ~FFmpegAudioOutput() = default;
    static FFmpegAudioOutput* create();
    virtual bool open(int channels, int sampleRate) = 0;
    virtual bool write(const uint8_t* data, size_t bytes) = 0;
    virtual void flush() = 0;
    virtual void interrupt() = 0;
};

} // namespace Starfish

#endif
