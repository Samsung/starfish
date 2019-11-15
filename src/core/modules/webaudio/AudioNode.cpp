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

#include "core/modules/webaudio/AudioNode.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/webaudio/AudioContext.h"

namespace Starfish {

AudioNode::AudioNode(ExecutionContext* executionContext)
    : AudioNode(executionContext, nullptr)
{
}

AudioNode::AudioNode(ExecutionContext* executionContext,
                     BaseAudioContext* context)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_context(context)
{
}

ScriptBindingInstance* AudioNode::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audionode-connect
AudioNode* AudioNode::connect(AudioNode* destinationNode, uint32_t output,
                              uint32_t input)
{
    if (destinationNode->context() != context()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }

    // TODO: Connect to destinationNode
    return destinationNode;
}

String* AudioNode::channelCountModeStr()
{
    switch (m_channelCountMode) {
    case ChannelCountMode::Max:
        return String::createASCIIString("max");
    case ChannelCountMode::ClampedMax:
        return String::createASCIIString("clamped-max");
    case ChannelCountMode::Explicit:
        return String::createASCIIString("explicit");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return String::emptyString;
}
void AudioNode::setChannelCountModeStr(String* channelCountMode)
{
    if (channelCountMode->equals("max")) {
        m_channelCountMode = ChannelCountMode::Max;
    } else if (channelCountMode->equals("clamped-max")) {
        m_channelCountMode = ChannelCountMode::ClampedMax;
    } else if (channelCountMode->equals("explicit")) {
        m_channelCountMode = ChannelCountMode::Explicit;
    }
}

String* AudioNode::channelInterpretationStr()
{
    switch (m_channelInterpretation) {
    case ChannelInterpretation::Speakers:
        return String::createASCIIString("speakers");
    case ChannelInterpretation::Discrete:
        return String::createASCIIString("discrete");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return String::emptyString;
}

void AudioNode::setChannelInterpretationStr(String* channelInterpretation)
{
    if (channelInterpretation->equals("speakers")) {
        m_channelInterpretation = ChannelInterpretation::Speakers;
    } else if (channelInterpretation->equals("discrete")) {
        m_channelInterpretation = ChannelInterpretation::Discrete;
    }
}
} // namespace Starfish

#endif
