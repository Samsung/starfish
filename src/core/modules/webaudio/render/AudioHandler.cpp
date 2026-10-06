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
#include "core/modules/webaudio/render/AudioHandler.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"

#include <algorithm>

namespace Starfish {

AudioHandler::AudioHandler(size_t inputs, size_t outputs, size_t outputChannels)
{
    for (size_t i = 0; i < inputs; i++) {
        m_inputs.emplace_back(new AudioNodeInput(this, i));
    }
    for (size_t i = 0; i < outputs; i++) {
        m_outputs.emplace_back(new AudioNodeOutput(this, outputChannels));
    }
}

AudioHandler::~AudioHandler() = default;

void AudioHandler::inputChannelsChanged(size_t)
{
}

AudioNodeInput& AudioHandler::input(size_t index)
{
    STARFISH_ASSERT(index < m_inputs.size());
    return *m_inputs[index];
}

AudioNodeOutput& AudioHandler::output(size_t index)
{
    STARFISH_ASSERT(index < m_outputs.size());
    return *m_outputs[index];
}

AudioParamTimeline* AudioHandler::timeline(size_t)
{
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

bool AudioHandler::hasPendingOutput() const
{
    for (const auto& output : m_outputs) {
        if (!output->bus().isSilent()) {
            return true;
        }
    }
    return false;
}

void AudioHandler::detachForTeardown()
{
    for (const auto& input : m_inputs) {
        input->clearForTeardown();
    }
    for (const auto& output : m_outputs) {
        output->clearForTeardown();
    }
    for (size_t index = 0; index < timelineCount(); index++) {
        timeline(index)->clearSourcesForTeardown();
    }
}

void AudioHandler::zeroOutputs()
{
    for (const auto& output : m_outputs) {
        output->bus().zero();
    }
}

void AudioHandler::renderMuted()
{
    zeroOutputs();
}

void AudioHandler::render(uint64_t frameStart, size_t frames)
{
    zeroOutputs();
    if (m_cycleMuted) {
        return;
    }
    for (const auto& input : m_inputs) {
        input->pull(frames);
    }
    process(frameStart, frames);
}

} // namespace Starfish

#endif
