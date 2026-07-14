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
#if !defined(STARFISH_TIZEN)

#ifndef __StarfishMediaPlayerWebRtcLinux__
#define __StarfishMediaPlayerWebRtcLinux__

#include "platform/multimedia/MediaPlayerWebRtc.h"

namespace Starfish {
class MediaPlayerWebRtcLinux : public MediaPlayerWebRtc {
    friend MediaStream;

public:
    MediaPlayerWebRtcLinux(HTMLMediaElement* element);
    virtual ~MediaPlayerWebRtcLinux();

    void destroy() override;
    void play() override;
    void pause() override;
    void seek(double time) override {};

    void prepare(MediaProvider* mediaProvider) override;

    void setVolume(double volume) override {};
    void setMuted(bool muted) override {};
    void prepareMediaSource() override;

    void onFrame(MediaStream::VideoFrameObserver* observer) override;
    void onData(MediaStream::AudioTrackObserver* observer) override;

private:
};
} // namespace Starfish

#endif
#endif
#endif
