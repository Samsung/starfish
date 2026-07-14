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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishMediaPlayerWebRtc__
#define __StarfishMediaPlayerWebRtc__

#include "platform/multimedia/MediaPlayer.h"

#include "core/dom/HTMLMediaElement.h"
#include "core/modules/mediastream/MediaStream.h"

namespace Starfish {
class HTMLMediaElement;
class Compositor;

class MediaPlayerWebRtc : public MediaPlayer {
    friend MediaStream;

public:
    virtual ~MediaPlayerWebRtc(){};

    static MediaPlayer* create(HTMLMediaElement* element);

    void destroy() override {};
    void play() override {};
    void pause() override {};
    void seek(double time) override {};

    virtual void prepare(MediaProvider* mediaProvider) {};
    bool isWebRtcPlayer() override
    {
        return true;
    }

    double currentTime() override
    {
        return 0;
    }

    double duration() override
    {
        return 0;
    }

    void setVolume(double volume) override {};
    void setMuted(bool muted) override {};
    void prepareMediaSource() override {};

    void didDrawVideo(Compositor* canvas, const LayoutRect& videoRect,
                      const LayoutRect& absVideoRect) override {};
    void willDrawVideo(Compositor* canvas,
                       const LayoutRect& videoRect) override {};

    virtual void onFrame(MediaStream::VideoFrameObserver* observer) = 0;
    virtual void onData(MediaStream::AudioTrackObserver* observer) = 0;

protected:
    MediaProvider* m_mediaProvider{ nullptr };

    MediaPlayerWebRtc(HTMLMediaElement* element);

private:
};
} // namespace Starfish

#endif
#endif
