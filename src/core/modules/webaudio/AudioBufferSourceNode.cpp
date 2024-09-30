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

#include "core/modules/webaudio/AudioBufferSourceNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/AudioDestinationNode.h"
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

// https://webaudio.github.io/web-audio-api/#dom-audiobuffersourcenode-buffe
void AudioBufferSourceNode::setBuffer(Optional<AudioBuffer*> buffer)
{
    if (buffer && m_bufferSet) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    if (buffer) {
        m_bufferSet = true;
        m_buffer = buffer;
    }
}

// https://webaudio.github.io/web-audio-api/#dom-audiobuffersourcenode-start
void AudioBufferSourceNode::start(double when, double offset, double duration)
{
    // 1
    if (m_hasStopCalled || m_hasStartCalled) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    // TODO: Timer
    if ((context()->controlQueue()->state() == AudioContextState::Suspended) &&
        context()->isAllowedToStart() && !context()->suspendedByUser()) {
        context()->renderingQueue()->enqueue(
            [](void* data) {
                AudioBufferSourceNode* self = (AudioBufferSourceNode*)data;
                self->m_hasStartCalled = true;
                if (self->m_destinationNode &&
                    self->m_destinationNode->isAudioDestinationNode()) {
                    AudioDestinationNode* destinationNode =
                        self->m_destinationNode->asAudioDestinationNode();
                    destinationNode->setBuffer(self->m_buffer->rawBuffer(),
                                               self->m_buffer->length());
                    destinationNode->play();
                }
                self->m_hasStartCalled = false;
            },
            this);
    }
}
} // namespace Starfish

#endif
