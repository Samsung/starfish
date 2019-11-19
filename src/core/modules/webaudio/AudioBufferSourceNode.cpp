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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/webaudio/AudioBufferSourceNode.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/BaseAudioContext.h"

#include "platform/multimedia/MediaPlayerAudio.h"

namespace Starfish {
AudioBufferSourceNode::AudioBufferSourceNode(ExecutionContext* executionContext,
                                             BaseAudioContext* context,
                                             AudioBufferSourceOptions options)
    : AudioScheduledSourceNode(executionContext, context)
{
    // https://webaudio.github.io/web-audio-api/#AudioBufferSourceNode
    m_numberOfInputs = 0;
    m_numberOfOutputs = 1;
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::Max;
    m_channelInterpretation = ChannelInterpretation::Speakers;

    m_buffer = options.m_buffer;
    if (m_buffer) {
        m_channelCount = m_buffer->numberOfChannels();
    }
}

ScriptBindingInstance* AudioBufferSourceNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audiobuffersourcenode-start
void AudioBufferSourceNode::start(double when, double offset, double duration)
{
    // TODO
    MediaPlayerAudio* player = MediaPlayerAudio::create(this);
    player->setBuffer(m_buffer->rawBuffer(), m_buffer->length());
    player->play();
}
} // namespace Starfish

#endif
