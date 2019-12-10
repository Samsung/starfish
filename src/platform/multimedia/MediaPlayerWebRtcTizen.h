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

#if defined(STARFISH_ENABLE_WEBRTC)
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)

#ifndef __StarfishMediaPlayerWebRtcTizen__
#define __StarfishMediaPlayerWebRtcTizen__

#include "platform/multimedia/MediaPlayerWebRtc.h"

#include <media/player.h>

namespace Starfish {
class HTMLMediaElement;
class Compositor;

class MediaPlayerWebRtcTizen : public MediaPlayerWebRtc {
    friend MediaStream;

public:
    MediaPlayerWebRtcTizen(HTMLMediaElement* element);
    virtual ~MediaPlayerWebRtcTizen();

    void destroy() override;
    void play() override;
    void pause() override{};
    void seek(double time) override{};

    void prepare(MediaProvider* mediaProvider) override;

    void setVolume(double volume) override{};
    void setMuted(bool muted) override{};
    void prepareMediaSource() override{};

    void onFrame(uint8_t* image) override;

private:
    player_h m_player{ nullptr };
    tbm_surface_h m_surface{ nullptr };
    tbm_surface_info_s m_surfaceInfo;

    void errorHandlerMediaFormat(int err, std::string msg);
    void errorHandlerMediaPacket(int err, std::string msg);
};
} // namespace Starfish

#endif
#endif
#endif
