/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

struct AudioBufferOptions {
    DEFINE_GETTER_SETTER(uint32_t, numberOfChannels, NumberOfChannels)
    DEFINE_GETTER_SETTER(uint32_t, length, Length)
    DEFINE_GETTER_SETTER(double, sampleRate, SampleRate)

    uint32_t m_numberOfChannels{ 1 };
    uint32_t m_length{ 0 };
    double m_sampleRate{ 0 };
};

class AudioBuffer : public ScriptWrappable {
public:
    AudioBuffer(ExecutionContext* executionContext, AudioBufferOptions options);
    AudioBuffer(ExecutionContext* executionContext,
                std::unique_ptr<uint8_t> buffer, uint32_t length);
    virtual ~AudioBuffer();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioBuffer)

    DEFINE_GETTER(double, sampleRate)
    DEFINE_GETTER(uint32_t, length)
    DEFINE_GETTER(double, duration)
    DEFINE_GETTER(uint32_t, numberOfChannels)

    uint8_t* rawBuffer();

private:
    ExecutionContext* m_executionContext{ nullptr };
    std::unique_ptr<uint8_t> m_buffer;
    double m_sampleRate{ 0 };
    uint32_t m_length{ 0 };
    double m_duration{ 0 };
    uint32_t m_numberOfChannels{ 1 };
};
}
#endif
#endif
