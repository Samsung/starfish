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

#include "core/modules/webaudio/AudioWorkletGlobalScope.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioNodeInput.h"
#include "core/modules/webaudio/render/AudioNodeOutput.h"

namespace Starfish {

AudioWorkletHandler::AudioWorkletHandler(size_t inputs,
                                         const std::vector<size_t>& outputs,
                                         bool outputFollowsInput)
    : AudioHandler(inputs, outputs.size(), 1)
    , m_outputFollowsInput(outputFollowsInput)
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

bool AudioWorkletHandler::hasActiveInput() const
{
    for (size_t index = 0; index < inputCount(); index++) {
        if (const_cast<AudioWorkletHandler*>(this)
                ->input(index)
                .hasActiveSource()) {
            return true;
        }
    }
    return false;
}

bool AudioWorkletHandler::isActivelyProcessing(uint64_t) const
{
    // https://webaudio.github.io/web-audio-api/#AudioNode-actively-processing
    // "when its AudioWorkletProcessor's [[callable process]] returns true
    // and either its active source flag is true or any AudioNode connected
    // to one of its inputs is actively processing."
    return m_callableProcess && (m_activeSourceFlag || hasActiveInput());
}

// The parameter values of this quantum, one array per AudioParam.
void AudioWorkletHandler::computeParameters(uint64_t frameStart)
{
    const size_t quantum = graph()->quantumFrames();
    if (!m_parameterValues) {
        m_parameterValues.reset(new float[m_timelines.size() * quantum]);
        m_constantParameters.resize(m_timelines.size());
    }
    for (size_t index = 0; index < m_timelines.size(); index++) {
        AudioParamTimeline* timeline = m_timelines[index].get();
        const AudioBus* modulation = timeline->modulation(quantum);
        float* values = m_parameterValues.get() + index * quantum;
        const size_t count = timeline->isARate() ? quantum : 1;
        bool constant = true;
        for (size_t frame = 0; frame < count; frame++) {
            values[frame] = timeline->computedValue(
                frameStart, frame, graph()->sampleRate(), modulation);
            constant = constant && values[frame] == values[0];
        }
        m_constantParameters[index] = constant;
    }
}

const float* AudioWorkletHandler::parameterValues(size_t index,
                                                  bool& constant) const
{
    constant = m_constantParameters[index];
    return m_parameterValues.get() + index * graph()->quantumFrames();
}

// https://webaudio.github.io/web-audio-api/#rendering-a-graph
void AudioWorkletHandler::process(uint64_t frameStart, size_t)
{
    if (m_outputFollowsInput) {
        // One input and one output without outputChannelCount: the output
        // has computedNumberOfChannels.
        output(0).configureChannels(input(0).bus().channels());
    }
    // The ED runs process() on the rendering thread. Starfish runs the
    // global scope on the main thread, so only a graph rendered there can
    // call it; otherwise the node outputs silence.
    if (!m_record || !m_callableProcess || !graph()->mainThreadRendering() ||
        (!m_activeSourceFlag && !hasActiveInput())) {
        return;
    }
    computeParameters(frameStart);
    m_record->scope->process(m_record, *this);
}
} // namespace Starfish

#endif
