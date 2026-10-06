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

#ifndef __StarfishAudioBufferSourceNode__
#define __StarfishAudioBufferSourceNode__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/webaudio/AudioScheduledSourceNode.h"

namespace Starfish {
class ExecutionContext;
class BaseAudioContext;
class AudioBuffer;
class AudioParam;
class AudioBufferSourceHandler;

struct AudioBufferSourceOptions {
    DEFINE_GETTER_SETTER(Optional<AudioBuffer*>, buffer, Buffer);
    DEFINE_GETTER_SETTER(float, detune, Detune);
    DEFINE_GETTER_SETTER(float, playbackRate, PlaybackRate);
    DEFINE_GETTER_SETTER(bool, loop, Loop);
    DEFINE_GETTER_SETTER(double, loopStart, LoopStart);
    DEFINE_GETTER_SETTER(double, loopEnd, LoopEnd);

    Optional<AudioBuffer*> m_buffer;
    float m_detune{ 0 };
    float m_playbackRate{ 1 };
    bool m_loop{ false };
    double m_loopStart{ 0 };
    double m_loopEnd{ 0 };
};

class AudioBufferSourceNode : public AudioScheduledSourceNode {
public:
    AudioBufferSourceNode(
        ExecutionContext* executionContext, BaseAudioContext* context,
        AudioBufferSourceOptions options = AudioBufferSourceOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioBufferSourceNode)

    DEFINE_GETTER(Optional<AudioBuffer*>, buffer);
    void setBuffer(Optional<AudioBuffer*> buffer);
    DEFINE_GETTER(AudioParam*, detune);
    DEFINE_GETTER(AudioParam*, playbackRate);
    DEFINE_GETTER(bool, loop);
    void setLoop(bool loop);
    DEFINE_GETTER(double, loopStart);
    void setLoopStart(double loopStart);
    DEFINE_GETTER(double, loopEnd);
    void setLoopEnd(double loopEnd);

    void start(double when = 0);
    void start(double when, double offset);
    void start(double when, double offset, double duration);
    void stop(double when = 0) override;
    bool renderScheduledQuantum() override;

private:
    Optional<AudioBuffer*> m_buffer;
    AudioParam* m_detune{ nullptr };
    AudioParam* m_playbackRate{ nullptr };
    bool m_loop{ false };
    double m_loopStart{ 0 };
    double m_loopEnd{ 0 };
    bool m_bufferSet{ false };
    double m_offset{ 0 };
    Optional<double> m_duration;
    AudioBufferSourceHandler* m_sourceHandler{ nullptr };
};
} // namespace Starfish
#endif
#endif
