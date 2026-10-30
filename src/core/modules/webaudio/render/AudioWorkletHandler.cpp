/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioWorkletHandler.h"
#include "core/modules/webaudio/render/AudioNodeOutput.h"

namespace Starfish {

AudioWorkletHandler::AudioWorkletHandler(size_t inputs,
                                         const std::vector<size_t>& outputs)
    : AudioHandler(inputs, outputs.size(), 1)
{
    for (size_t index = 0; index < outputs.size(); index++) {
        output(index).configureChannels(outputs[index]);
    }
}

AudioParamTimeline* AudioWorkletHandler::addTimeline(float defaultValue,
                                                     float minValue,
                                                     float maxValue,
                                                     bool isARate)
{
    std::unique_ptr<AudioParamTimeline> timeline(
        new AudioParamTimeline(defaultValue, minValue, maxValue));
    timeline->setARate(isARate);
    m_timelines.push_back(std::move(timeline));
    return m_timelines.back().get();
}

void AudioWorkletHandler::process(uint64_t, size_t)
{
    // The processor's process() is not called yet: the outputs stay silent.
}
} // namespace Starfish

#endif
