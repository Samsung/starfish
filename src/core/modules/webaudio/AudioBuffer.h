/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioBuffer__
#define __StarfishAudioBuffer__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/webaudio/BaseAudioContext.h"

namespace Starfish {
class ExecutionContext;
class AudioBufferData;

struct AudioBufferOptions {
    DEFINE_GETTER_SETTER(uint32_t, numberOfChannels, NumberOfChannels)
    DEFINE_GETTER(uint32_t, length)
    DEFINE_GETTER(double, sampleRate)

    void setLength(uint32_t value)
    {
        m_length = value;
        m_hasLength = true;
    }
    void setSampleRate(double value)
    {
        m_sampleRate = value;
        m_hasSampleRate = true;
    }
    bool hasLength() const
    {
        return m_hasLength;
    }
    bool hasSampleRate() const
    {
        return m_hasSampleRate;
    }

    uint32_t m_numberOfChannels{ 1 };
    uint32_t m_length{ 0 };
    double m_sampleRate{ 0 };
    bool m_hasLength{ false };
    bool m_hasSampleRate{ false };
};

class AudioBuffer : public ScriptWrappable {
public:
    AudioBuffer(ExecutionContext* executionContext, AudioBufferOptions options);
    AudioBuffer(ExecutionContext* executionContext, AudioBufferData* data,
                double sampleRate);
    virtual ~AudioBuffer();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioBuffer)

    DEFINE_GETTER(double, sampleRate)
    DEFINE_GETTER(uint32_t, length)
    DEFINE_GETTER(double, duration)
    DEFINE_GETTER(uint32_t, numberOfChannels)

    ScriptFloat32Array getChannelData(uint32_t channel);
    void copyFromChannel(ScriptFloat32Array destination, uint32_t channelNumber,
                         uint32_t bufferOffset = 0);
    void copyToChannel(ScriptFloat32Array source, uint32_t channelNumber,
                       uint32_t bufferOffset = 0);
    void acquireContents();

    AudioBufferData* data() const
    {
        return m_data;
    }

private:
    void ensureMutableData();

    ExecutionContext* m_executionContext{ nullptr };
    AudioBufferData* m_data{ nullptr };
    GCVector<ScriptFloat32Array> m_channelViews;
    bool m_dataAcquired{ false };
    double m_sampleRate{ 0 };
    uint32_t m_length{ 0 };
    double m_duration{ 0 };
    uint32_t m_numberOfChannels{ 1 };
};
} // namespace Starfish
#endif
#endif
