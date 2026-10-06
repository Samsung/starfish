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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioGraph.h"

#include "core/modules/webaudio/AudioBufferSourceNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <cmath>

namespace Starfish {
AudioBufferSourceNode::AudioBufferSourceNode(ExecutionContext* executionContext,
                                             BaseAudioContext* context,
                                             AudioBufferSourceOptions options)
    : AudioScheduledSourceNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    // https://webaudio.github.io/web-audio-api/#AudioBufferSourceNode
    if (!std::isfinite(options.detune()) ||
        !std::isfinite(options.playbackRate())) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Buffer source rate must be finite");
    }
    m_numberOfInputs = 0;
    m_numberOfOutputs = 1;
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::Max;
    m_channelInterpretation = ChannelInterpretation::Speakers;

    m_sourceHandler = static_cast<AudioBufferSourceHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new AudioBufferSourceHandler(context->sampleRate()))));
    m_handler = m_sourceHandler;
    m_detune = new AudioParam(context, m_sourceHandler->detuneTimeline());
    m_playbackRate =
        new AudioParam(context, m_sourceHandler->playbackRateTimeline());
    context->graph()->registerTimeline(m_sourceHandler->detuneTimeline());
    context->graph()->registerTimeline(m_sourceHandler->playbackRateTimeline());
    m_sourceHandler->detuneTimeline()->setInitialValue(options.detune());
    m_sourceHandler->playbackRateTimeline()->setInitialValue(
        options.playbackRate());
    m_sourceHandler->detuneTimeline()->setFixedKRate();
    m_sourceHandler->playbackRateTimeline()->setFixedKRate();

    m_loop = options.loop();
    m_loopStart = options.loopStart();
    m_loopEnd = options.loopEnd();
    m_sourceHandler->setLoop(m_loop, m_loopStart, m_loopEnd);

    m_buffer = options.m_buffer;
    if (m_buffer) {
        m_bufferSet = true;
        m_channelCount = m_buffer->numberOfChannels();
        m_sourceHandler->setBuffer(m_buffer->data(), m_buffer->sampleRate());
    }
}

ScriptBindingInstance* AudioBufferSourceNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audiobuffersourcenode-buffe
void AudioBufferSourceNode::setBuffer(Optional<AudioBuffer*> buffer)
{
    AudioGraphLock graphLock(context()->graph());
    if (buffer && m_bufferSet) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    if (buffer) {
        m_bufferSet = true;
    }
    m_buffer = buffer;
    m_sourceHandler->setBuffer(m_buffer ? m_buffer->data() : nullptr,
                               m_buffer ? m_buffer->sampleRate() : 0);
    if (m_buffer && m_hasStartCalled) {
        m_buffer->acquireContents();
    }
}

void AudioBufferSourceNode::setLoop(bool loop)
{
    AudioGraphLock graphLock(context()->graph());
    m_loop = loop;
    m_sourceHandler->setLoop(m_loop, m_loopStart, m_loopEnd);
}

void AudioBufferSourceNode::setLoopStart(double loopStart)
{
    AudioGraphLock graphLock(context()->graph());
    m_loopStart = loopStart;
    m_sourceHandler->setLoop(m_loop, m_loopStart, m_loopEnd);
}

void AudioBufferSourceNode::setLoopEnd(double loopEnd)
{
    AudioGraphLock graphLock(context()->graph());
    m_loopEnd = loopEnd;
    m_sourceHandler->setLoop(m_loop, m_loopStart, m_loopEnd);
}

// https://webaudio.github.io/web-audio-api/#dom-audiobuffersourcenode-start
void AudioBufferSourceNode::start(double when)
{
    AudioGraphLock graphLock(context()->graph());
    start(when, 0);
}

void AudioBufferSourceNode::start(double when, double offset)
{
    AudioGraphLock graphLock(context()->graph());
    if (offset < 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Offset must be non-negative");
    }
    AudioScheduledSourceNode::start(when);
    if (m_buffer) {
        // Another node may have acquired this buffer since assignment, and
        // script may have replaced its PCM through copy-on-write meanwhile.
        // https://webaudio.github.io/web-audio-api/#acquire-the-content
        m_sourceHandler->setBuffer(m_buffer->data(), m_buffer->sampleRate());
        m_buffer->acquireContents();
    }
    m_offset = offset;
    m_sourceHandler->start(when, offset, false, 0);
    context()->registerScheduledSource(this);
}

void AudioBufferSourceNode::start(double when, double offset, double duration)
{
    AudioGraphLock graphLock(context()->graph());
    if (duration < 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Duration must be non-negative");
    }
    start(when, offset);
    m_duration = duration;
    m_sourceHandler->start(when, offset, true, duration);
}

void AudioBufferSourceNode::stop(double when)
{
    AudioGraphLock graphLock(context()->graph());
    AudioScheduledSourceNode::stop(when);
    m_sourceHandler->stop(when);
}

bool AudioBufferSourceNode::renderScheduledQuantum()
{
    AudioGraphLock graphLock(context()->graph());
    return m_sourceHandler->finished();
}
} // namespace Starfish

#endif
