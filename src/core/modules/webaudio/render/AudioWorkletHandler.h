/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishAudioWorkletHandler__
#define __StarfishAudioWorkletHandler__

#include "core/modules/webaudio/render/AudioHandler.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"

#include <memory>
#include <vector>

namespace Starfish {
class AudioWorkletProcessorRecord;

// https://webaudio.github.io/web-audio-api/#AudioWorkletNode
// The render side of an AudioWorkletNode: its AudioParam timelines, its
// processor's flags, and the global scope's record of the processor, which
// the scope keeps until the graph deletes this handler.
class AudioWorkletHandler final : public AudioHandler {
public:
    AudioWorkletHandler(size_t inputs, const std::vector<size_t>& outputs,
                        bool outputFollowsInput);

    AudioParamTimeline* addTimeline(float defaultValue, float minValue,
                                    float maxValue, bool isARate);
    size_t timelineCount() const override
    {
        return m_timelines.size();
    }
    AudioParamTimeline* timeline(size_t index) override
    {
        return m_timelines[index].get();
    }
    void setRecord(AudioWorkletProcessorRecord* record)
    {
        m_record = record;
    }
    void* workletRecord() const override
    {
        return m_record;
    }
    // https://webaudio.github.io/web-audio-api/#active-source
    // Keeps the node rendering, and alive, without active inputs.
    void setActiveSourceFlag(bool active)
    {
        m_activeSourceFlag = active;
    }
    // [[callable process]] became false: process() threw.
    void clearCallableProcess()
    {
        m_callableProcess = false;
    }
    bool hasPendingOutput() const override
    {
        return m_activeSourceFlag || AudioHandler::hasPendingOutput();
    }
    bool hasActiveInput() const;
    // This quantum's values of parameter |index|; |constant| when every
    // frame has values[0].
    const float* parameterValues(size_t index, bool& constant) const;

protected:
    void process(uint64_t frameStart, size_t frames) override;
    bool isActivelyProcessing(uint64_t quantumEnd) const override;

private:
    void computeParameters(uint64_t frameStart);

    std::vector<std::unique_ptr<AudioParamTimeline>> m_timelines;
    // Parameters × render quantum floats, allocated on the first process().
    std::unique_ptr<float[]> m_parameterValues;
    std::vector<bool> m_constantParameters;
    AudioWorkletProcessorRecord* m_record{ nullptr };
    bool m_outputFollowsInput;
    bool m_activeSourceFlag{ true };
    bool m_callableProcess{ true };
};
} // namespace Starfish

#endif
#endif
