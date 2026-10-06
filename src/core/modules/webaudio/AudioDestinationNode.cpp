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

#include "core/modules/webaudio/AudioDestinationNode.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

namespace Starfish {

AudioDestinationNode::AudioDestinationNode(ExecutionContext* executionContext)
    : AudioDestinationNode(executionContext, nullptr)
{
}

AudioDestinationNode::AudioDestinationNode(ExecutionContext* executionContext,
                                           BaseAudioContext* context)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    m_numberOfInputs = 1;
    m_numberOfOutputs = 0;
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_destinationHandler =
        static_cast<AudioDestinationHandler*>(context->graph()->addHandler(
            std::unique_ptr<AudioHandler>(new AudioDestinationHandler(2))));
    m_handler = m_destinationHandler;
    context->graph()->setDestination(&m_destinationHandler->output(0));
}

void AudioDestinationNode::configureOfflineChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    m_maxChannelCount = value;
    m_channelCount = value;
    m_channelCountFixed = true;
    m_destinationHandler->configureChannels(value);
}

// https://webaudio.github.io/web-audio-api/#dom-audionode-channelcount
void AudioDestinationNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    // The generic AudioNode rule applies first: zero is NotSupportedError.
    if (!value) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Destination channel count must be positive");
    }
    // OfflineAudioContext destination: "The channel count cannot be changed.
    // An InvalidStateError exception MUST be thrown for any attempt to change
    // the value." (The AudioDestinationNode prose names NotSupportedError;
    // the normative channelCount attribute definition is followed here.)
    if (m_channelCountFixed) {
        if (value != m_channelCount) {
            throw new DOMException(
                executionContext(), DOMException::INVALID_STATE_ERR,
                "OfflineAudioContext destination channel count is fixed");
        }
        return;
    }
    // AudioContext destination: outside [1, maxChannelCount] is
    // IndexSizeError.
    if (value > m_maxChannelCount) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "Destination channel count exceeds maximum");
    }
    m_channelCount = value;
    m_destinationHandler->configureChannels(value);
}

ScriptBindingInstance* AudioDestinationNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
