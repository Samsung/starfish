/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioBus__
#define __StarfishAudioBus__

#include <cstddef>
#include <memory>

namespace Starfish {

// Non-GC planar audio storage. Resize only while configuring the graph.
// Shrinking keeps the allocation, so toggling a channel count (for example a
// ChannelMergerNode going inactive) does not allocate on the render thread.
class AudioBus {
public:
    static const size_t RenderQuantumFrames = 128;

    AudioBus(size_t channels = 0, size_t frames = RenderQuantumFrames);

    void resize(size_t channels, size_t frames = RenderQuantumFrames);
    void zero();
    void addFrom(const AudioBus& source, size_t frames, bool speakers);
    void copyFrom(const AudioBus& source, size_t frames, bool speakers);

    float* channel(size_t index);
    const float* channel(size_t index) const;
    size_t channels() const
    {
        return m_channels;
    }
    size_t frames() const
    {
        return m_frames;
    }
    bool isSilent() const
    {
        return m_isSilent;
    }
    void setSilent(bool silent)
    {
        m_isSilent = silent;
    }

private:
    std::unique_ptr<float[]> m_data;
    size_t m_capacity{ 0 };
    size_t m_channels{ 0 };
    size_t m_frames{ 0 };
    bool m_isSilent{ true };
};

} // namespace Starfish

#endif
#endif
