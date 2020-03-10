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
#if defined(STARFISH_USE_MOCK_MEDIAPLAYER) || !defined(STARFISH_TIZEN)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "MediaPlayerAudioMock.h"

#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/dom/HTMLMediaElement.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"

namespace Starfish {
MediaPlayerAudioMock::MediaPlayerAudioMock(AudioNode* element)
    : MediaPlayerAudio(element)
{
}

MediaPlayerAudioMock::MediaPlayerAudioMock(HTMLMediaElement* element)
    : MediaPlayerAudio(element)
{
}

void MediaPlayerAudioMock::play()
{
    STARFISH_LOG_INFO("%s\n", __func__);
}

void MediaPlayerAudioMock::destroy()
{
    STARFISH_LOG_INFO("%s\n", __func__);
}

void MediaPlayerAudioMock::prepare(ResourceURL* url)
{
}

MediaPlayerAudio* MediaPlayerAudio::create(HTMLMediaElement* element)
{
    return new MediaPlayerAudioMock(element);
}

MediaPlayerAudio* MediaPlayerAudio::create(AudioNode* element)
{
    return new MediaPlayerAudioMock(element);
}
} // namespace Starfish

#endif
#endif
