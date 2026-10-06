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

namespace Starfish {

// Shared by AudioBuffer and channel ArrayBuffer backing stores. This object
// contains no GC pointers, so the rendering side can safely retain it.
class AudioBufferData {
public:
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
    AudioBufferData(float* samples, size_t channels, size_t frames);
    ~AudioBufferData();

    std::atomic<unsigned> m_references{ 1 };
    std::atomic<unsigned> m_viewReferences{ 0 };
    float* m_samples;
    size_t m_channels;
    size_t m_frames;
};

} // namespace Starfish

#endif
#endif
