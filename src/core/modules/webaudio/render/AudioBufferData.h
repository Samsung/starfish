/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioBufferData__
#define __StarfishAudioBufferData__

#include <atomic>
#include <cstddef>
#include <vector>

namespace Starfish {

// Shared by AudioBuffer and channel ArrayBuffer backing stores. This object
// contains no GC pointers, so the rendering side can safely retain it.
class AudioBufferData {
public:
    // Limit a single web-controlled allocation to preserve the low-memory
    // embedding profile; longer audio should use streaming media.
    static constexpr size_t MaxSamples = 128 * 1024 * 1024 / sizeof(float);

    static AudioBufferData* create(size_t channels, size_t frames);

    void retain();
    void release();
    // References held by channel ArrayBuffer backing stores. They are dropped
    // only when the GC collects the store, possibly long after detachment.
    void retainForView();
    void releaseForView();
    // True when no reference other than the caller's and those of channel
    // views exists. Call only from the main thread, which alone adds
    // references, and only while every view is detached. It may report false
    // spuriously while another thread is releasing, never true spuriously.
    bool isOnlyReferencedByOwnerAndViews() const;

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

private:
    friend class AudioBufferDataBuilder;

    AudioBufferData(float** channelData, size_t channels, size_t frames);
    ~AudioBufferData();

    std::atomic<unsigned> m_references{ 1 };
    std::atomic<unsigned> m_viewReferences{ 0 };
    float** m_channelData;
    size_t m_channels;
    size_t m_frames;
};

// Collects decoded PCM straight into the buffers the finished AudioBufferData
// adopts. A decoder learns the length only at the end; copying from a staging
// buffer would hold the PCM twice.
class AudioBufferDataBuilder {
public:
    // `expectedFrames` (0 if unknown) sizes the first allocation, so a
    // correct estimate never reallocates.
    AudioBufferDataBuilder(size_t channels, size_t expectedFrames);
    ~AudioBufferDataBuilder();

    size_t channels() const
    {
        return m_channelData.size();
    }
    size_t frames() const
    {
        return m_frames;
    }
    // Makes room for `frames` more frames per channel; false past the
    // AudioBuffer limit or on allocation failure.
    bool reserve(size_t frames);
    // The first unwritten frame of `channel`; valid until the next reserve().
    float* end(size_t channel)
    {
        return m_channelData[channel] + m_frames;
    }
    // Marks `frames` reserved frames written.
    void commit(size_t frames)
    {
        m_frames += frames;
    }
    // Returns the buffer, or nullptr when nothing was written.
    AudioBufferData* finish();

private:
    std::vector<float*> m_channelData;
    size_t m_expectedFrames;
    size_t m_capacity{ 0 };
    size_t m_frames{ 0 };
};

} // namespace Starfish

#endif
#endif
