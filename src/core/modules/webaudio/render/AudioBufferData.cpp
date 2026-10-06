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
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <cstdlib>
#include <limits>

namespace Starfish {

AudioBufferData* AudioBufferData::create(size_t channels, size_t frames)
{
    // Limit a single web-controlled allocation to preserve the low-memory
    // embedding profile; longer audio should use streaming media.
    static const size_t maxBytes = 128 * 1024 * 1024;
    if (!channels || !frames ||
        channels >
            std::numeric_limits<size_t>::max() / frames / sizeof(float) ||
        channels * frames > maxBytes / sizeof(float)) {
        return nullptr;
    }

    float* samples =
        static_cast<float*>(calloc(channels * frames, sizeof(float)));
    if (!samples) {
        return nullptr;
    }
    return new AudioBufferData(samples, channels, frames);
}

AudioBufferData::AudioBufferData(float* samples, size_t channels, size_t frames)
    : m_samples(samples)
    , m_channels(channels)
    , m_frames(frames)
{
}

AudioBufferData::~AudioBufferData()
{
    free(m_samples);
}

void AudioBufferData::retain()
{
    m_references.fetch_add(1, std::memory_order_relaxed);
}

void AudioBufferData::release()
{
    if (m_references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

void AudioBufferData::retainForView()
{
    m_viewReferences.fetch_add(1, std::memory_order_relaxed);
    retain();
}

void AudioBufferData::releaseForView()
{
    // Drop the view count before the reference itself, so a concurrent
    // isOnlyReferencedByOwnerAndViews() can only overcount other owners.
    m_viewReferences.fetch_sub(1, std::memory_order_release);
    release();
}

bool AudioBufferData::isOnlyReferencedByOwnerAndViews() const
{
    // Read the total first: view counts only decrease after it, so the
    // difference never undercounts non-view references. The acquire load
    // pairs with release() on the render thread, ordering its last reads
    // before the caller's writes.
    const unsigned references = m_references.load(std::memory_order_acquire);
    const unsigned views = m_viewReferences.load(std::memory_order_acquire);
    return references - views == 1;
}

float* AudioBufferData::channel(size_t index)
{
    STARFISH_ASSERT(index < m_channels);
    return m_samples + index * m_frames;
}

const float* AudioBufferData::channel(size_t index) const
{
    STARFISH_ASSERT(index < m_channels);
    return m_samples + index * m_frames;
}

} // namespace Starfish

#endif
