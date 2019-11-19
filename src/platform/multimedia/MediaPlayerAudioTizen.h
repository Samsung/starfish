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
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)

#ifndef __StarfishMediaPlayerAudioTizen__
#define __StarfishMediaPlayerAudioTizen__

#include "platform/multimedia/MediaPlayerAudio.h"

#include <media/player.h>

namespace Starfish {
class HTMLMediaElement;
class Compositor;

class MediaPlayerAudioTizen : public MediaPlayerAudio {
public:
    MediaPlayerAudioTizen(AudioNode* element);
    MediaPlayerAudioTizen(HTMLMediaElement* element);
    virtual ~MediaPlayerAudioTizen();

    virtual void destroy() override;
    virtual void play() override;
    virtual void pause() override{};
    virtual void seek(double time) override{};

    virtual void setBuffer(uint8_t* buffer, uint32_t length) override;
    virtual void prepare(ResourceURL* url) override;

    virtual double currentTime()
    {
        return 0;
    }

    virtual double duration()
    {
        return 0;
    }

    virtual void setVolume(double volume) override{};
    virtual void setMuted(bool muted) override{};
    virtual void prepareMediaSource() override{};

private:
    player_h m_player{ nullptr };
};
}

#endif
#endif
#endif
