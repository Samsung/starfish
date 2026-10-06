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

#include "core/modules/webaudio/MediaElementAudioSourceNode.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/webaudio/AudioNode.h"
#include "core/modules/webaudio/AudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"
#include "platform/multimedia/MediaPlayer.h"

namespace Starfish {
MediaElementAudioSourceNode::MediaElementAudioSourceNode(
    ExecutionContext* executionContext, AudioContext* context,
    MediaElementAudioSourceOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    m_mediaElement = options.m_mediaElement;
    if (!m_mediaElement) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "mediaElement is required");
    }
    if (m_mediaElement->audioSourceNode()) {
        throw new DOMException(executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "Media element already has an audio source");
    }
    m_mediaElement->setAudioSourceNode(this);
    m_numberOfInputs = 0;
    m_numberOfOutputs = 1;
    m_handler = context->graph()->addHandler(std::unique_ptr<AudioHandler>(
        new MediaElementSourceHandler(context->sampleRate())));
    m_sourceHandler = static_cast<MediaElementSourceHandler*>(m_handler);
    MediaPlayer* player = m_mediaElement->activeMediaPlayer();
    if (player) {
        setPlaybackState(player->audioPlaybackState());
    }
}

void MediaElementAudioSourceNode::setPlaybackState(
    MediaAudioPlaybackState* state)
{
    AudioGraphLock graphLock(context()->graph());
    m_sourceHandler->setPlaybackState(state);
}

ScriptBindingInstance* MediaElementAudioSourceNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish

#endif
