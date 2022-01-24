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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && defined(STARFISH_ENABLE_WEBAUDIO)
#if !defined(STARFISH_TIZEN)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "MediaPlayerAudioLinux.h"

#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/dom/HTMLMediaElement.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"

namespace Starfish {
MediaPlayerAudioLinux::MediaPlayerAudioLinux(AudioNode* element)
    : MediaPlayerAudio(element)
{
}

MediaPlayerAudioLinux::MediaPlayerAudioLinux(HTMLMediaElement* element)
    : MediaPlayerAudio(element)
{
}

void MediaPlayerAudioLinux::play()
{
    STARFISH_LOG_INFO("MediaPlayerAudioLinux::%s", __func__);
    MediaPlayerAudio::play();
}

void MediaPlayerAudioLinux::destroy()
{
    STARFISH_LOG_INFO("MediaPlayerAudioLinux::%s", __func__);
    MediaPlayerAudio::destroy();
}

void MediaPlayerAudioLinux::prepare(ResourceURL* url)
{
    STARFISH_LOG_INFO("MediaPlayerAudioLinux::%s", __func__);
    MediaPlayerAudio::prepare(url);
}

MediaPlayerAudio* MediaPlayerAudio::create(HTMLMediaElement* element)
{
    return new MediaPlayerAudioLinux(element);
}

MediaPlayerAudio* MediaPlayerAudio::create(AudioNode* element)
{
    return new MediaPlayerAudioLinux(element);
}
} // namespace Starfish

#endif
#endif
