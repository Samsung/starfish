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

namespace Starfish {
AudioBufferSourceNode::AudioBufferSourceNode(ExecutionContext* executionContext,
                                             BaseAudioContext* context,
                                             AudioBufferSourceOptions options)
    : AudioScheduledSourceNode(executionContext)
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
    return m_executionContext->scriptBindingInstance();
}
} // namespace Starfish

#endif
