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

#ifndef __StarfishMediaPlayerWebRtc__
#define __StarfishMediaPlayerWebRtc__

#include "platform/multimedia/MediaPlayer.h"

namespace Starfish {
class HTMLMediaElement;

class MediaPlayerWebRtc : public MediaPlayer {
    friend MediaStream;

public:
    MediaPlayerWebRtc(HTMLMediaElement* element);
    virtual ~MediaPlayerWebRtc(){};

    static MediaPlayer* create(HTMLMediaElement* element);

    virtual void destroy(){};
    virtual void play();
    virtual void pause(){};
    virtual void seek(double time){};

    void prepare(MediaProvider* mediaProvider);
    virtual bool isWebRtcPlayer()
    {
        return true;
    }

    virtual double currentTime()
    {
        return 0;
    }

    virtual double duration()
    {
        return 0;
    }

    virtual void setVolume(double volume){};
    virtual void setMuted(bool muted){};
    virtual void prepareMediaSource();

    virtual void didDrawVideo(Compositor* canvas, const LayoutRect& videoRect,
                              const LayoutRect& absVideoRect);
    virtual void willDrawVideo(Compositor* canvas, const LayoutRect& videoRect);

    void onFrame(uint8_t* image);

private:
};
}

#endif
#endif
