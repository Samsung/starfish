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

#include "core/modules/webaudio/BaseAudioContext.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"
#include "core/modules/webaudio/AudioDestinationNode.h"

namespace Starfish {
BaseAudioContext::BaseAudioContext(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
{
}

ScriptBindingInstance* BaseAudioContext::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

AudioDestinationNode* BaseAudioContext::destination()
{
    // TODO: destination represents the actual hardware output stream
    AudioDestinationNode* dest =
        new AudioDestinationNode(m_executionContext, this);
    return dest;
}

String* BaseAudioContext::state()
{
    switch (m_state) {
    case AudioContextState::Suspended:
        return String::createASCIIString("suspended");
    case AudioContextState::Running:
        return String::createASCIIString("running");
    case AudioContextState::Closed:
        return String::createASCIIString("closed");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return String::emptyString;
}

AudioBufferSourceNode* BaseAudioContext::createBufferSource()
{
    AudioBufferSourceOptions options;
    AudioBufferSourceNode* node =
        new AudioBufferSourceNode(m_executionContext, this, options);

    return node;
}
} // namespace Starfish

#endif
