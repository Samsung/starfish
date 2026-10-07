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

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <new>

namespace Starfish {

namespace {

    bool isWithinLimit(size_t channels, size_t frames)
    {
        return channels && frames &&
               channels <= AudioBufferData::MaxSamples / frames;
    }

    void freeChannels(float* const* channelData, size_t channels)
    {
        for (size_t channel = 0; channel < channels; channel++) {
            free(channelData[channel]);
        }
    }

} // namespace

AudioBufferData* AudioBufferData::create(size_t channels, size_t frames)
{
    if (!isWithinLimit(channels, frames)) {
        return nullptr;
    }
    std::unique_ptr<float*[]> channelData(
        new (std::nothrow) float*[channels]());
    if (!channelData) {
        return nullptr;
    }
    for (size_t channel = 0; channel < channels; channel++) {
        channelData[channel] =
            static_cast<float*>(calloc(frames, sizeof(float)));
        if (!channelData[channel]) {
            freeChannels(channelData.get(), channel);
            return nullptr;
        }
    }
    return new AudioBufferData(channelData.release(), channels, frames);
}

AudioBufferData* AudioBufferData::adopt(float* const* channelData,
                                        size_t channels, size_t frames)
{
    std::unique_ptr<float*[]> owned;
    if (isWithinLimit(channels, frames)) {
        owned.reset(new (std::nothrow) float*[channels]);
    }
    if (!owned) {
        freeChannels(channelData, channels);
        return nullptr;
    }
    std::copy(channelData, channelData + channels, owned.get());
    return new AudioBufferData(owned.release(), channels, frames);
}

AudioBufferData::AudioBufferData(float** channelData, size_t channels,
                                 size_t frames)
    : m_channelData(channelData)
    , m_channels(channels)
    , m_frames(frames)
{
}

AudioBufferData::~AudioBufferData()
{
    freeChannels(m_channelData, m_channels);
    delete[] m_channelData;
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
    return m_channelData[index];
}

const float* AudioBufferData::channel(size_t index) const
{
    STARFISH_ASSERT(index < m_channels);
    return m_channelData[index];
}

} // namespace Starfish

#endif
