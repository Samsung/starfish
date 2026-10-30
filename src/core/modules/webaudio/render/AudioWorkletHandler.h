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
// The render side of an AudioWorkletNode: its AudioParam timelines, and the
// global scope's record of its processor, which the scope keeps until the
// graph deletes this handler.
class AudioWorkletHandler final : public AudioHandler {
public:
    AudioWorkletHandler(size_t inputs, const std::vector<size_t>& outputs);

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

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    std::vector<std::unique_ptr<AudioParamTimeline>> m_timelines;
    AudioWorkletProcessorRecord* m_record{ nullptr };
};
} // namespace Starfish

#endif
#endif
