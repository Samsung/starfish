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

#ifndef __StarfishMediaStream__
#define __StarfishMediaStream__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "api/media_stream_interface.h"
#include "pc/video_track_source.h"
#include "platform/webrtc/VideoCapturer.h"

#include "core/modules/mediastream/MediaStreamTrack.h"

namespace Starfish {
class ExecutionContext;
class MediaPlayerWebRtc;

class MediaStream : public EventTarget, public MediaStreamTrackObserver {
public:
    class MediaStreamObserver
        : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    public:
        MediaStreamObserver(webrtc::VideoTrackInterface* trackToRender,
                            MediaPlayerWebRtc* player);
        virtual ~MediaStreamObserver();

        // VideoSinkInterface implementation
        void OnFrame(const webrtc::VideoFrame& frame) override;

        MediaPlayerWebRtc* m_player;

    private:
        void setSize(int width, int height);

        std::unique_ptr<uint8_t[]> m_image;
        int m_width{ 0 };
        int m_height{ 0 };
        rtc::scoped_refptr<webrtc::VideoTrackInterface> m_trackToRender;
    };

    const std::string m_mediaStreamLabel = "MediaStream";

    MediaStream(ExecutionContext* executionContext);
    MediaStream(ExecutionContext* executionContext,
                rtc::scoped_refptr<webrtc::MediaStreamInterface> backend);
    MediaStream(ExecutionContext* executionContext, MediaStream& mediaStream);
    MediaStream(ExecutionContext* executionContext,
                GCVector<MediaStreamTrack*>& tracks);
    virtual ~MediaStream();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaStream)
    virtual ExecutionContext* executionContext() const override;

    rtc::scoped_refptr<webrtc::MediaStreamInterface> backend()
    {
        return m_backend;
    }

    GCVector<MediaStreamTrack*> getAudioTracks();
    GCVector<MediaStreamTrack*> getVideoTracks();
    GCVector<MediaStreamTrack*> getTracks();
    void addTrack(MediaStreamTrack* track);
    void removeTrack(MediaStreamTrack* track);

    void removeAudioTrack(AudioStreamTrack* track) override;
    void removeVideoTrack(VideoStreamTrack* track) override;

    void startPlayVideoTrack(MediaPlayerWebRtc* player,
                             MediaStreamTrack* track);
    void stopPlayVideoTrack();

private:
    ExecutionContext* m_executionContext{ nullptr };
    rtc::scoped_refptr<webrtc::MediaStreamInterface> m_backend;
    GCUnorderedSet<AudioStreamTrack*> m_audioTracks;
    GCUnorderedSet<VideoStreamTrack*> m_videoTracks;

    std::unique_ptr<MediaStreamObserver> m_mediaStreamObserver;
};
} // namespace Starfish

#endif
#endif
