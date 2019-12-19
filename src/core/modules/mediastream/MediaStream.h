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
    static const int PIXEL_STRIDE = 4;

    class VideoFrameObserver
        : public gc,
          public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    public:
        VideoFrameObserver(webrtc::VideoTrackInterface* videoTrack,
                           MediaPlayerWebRtc* player);
        virtual ~VideoFrameObserver();

        // VideoSinkInterface implementation
        void OnFrame(const webrtc::VideoFrame& frame) override;

        void stop();

        uint8_t* image()
        {
            return m_image.get();
        }

        int width()
        {
            return m_width;
        }

        int height()
        {
            return m_height;
        }

        int pixelStride()
        {
            return PIXEL_STRIDE;
        }

    private:
        void setSize(int width, int height);
#if defined(STARFISH_WEBRTC_DEBUG)
        void writeImageToFile(std::string& filename);
#endif
        MediaPlayerWebRtc* m_player{ nullptr };

        std::unique_ptr<uint8_t[]> m_image;
        int m_width{ 0 };
        int m_height{ 0 };
        rtc::scoped_refptr<webrtc::VideoTrackInterface> m_videoTrack;
    };

    class AudioTrackObserver : public gc,
                               public webrtc::AudioTrackSinkInterface {
    public:
        AudioTrackObserver(webrtc::AudioTrackInterface* audioTrack,
                           MediaPlayerWebRtc* player);
        virtual ~AudioTrackObserver();

        // AudioAudioTrackSinkInterface implementation
        void OnData(const void* audioData, int bitsPerSample, int sampleRate,
                    size_t numberOfChannels, size_t numberOfFrames) override;

        void stop();

    private:
        rtc::scoped_refptr<webrtc::AudioTrackInterface> m_audioTrack;
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

    String* id();
    GCVector<MediaStreamTrack*> getAudioTracks();
    GCVector<MediaStreamTrack*> getVideoTracks();
    GCVector<MediaStreamTrack*> getTracks();
    void addTrack(MediaStreamTrack* track);
    void removeTrack(MediaStreamTrack* track);

    void removeAudioTrack(AudioStreamTrack* track) override;
    void removeVideoTrack(VideoStreamTrack* track) override;

    void playTrack(MediaPlayerWebRtc* player, MediaStreamTrack* track);
    void stopTrack();

    void syncTracks();

    rtc::scoped_refptr<webrtc::MediaStreamInterface> backend()
    {
        return m_backend;
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    rtc::scoped_refptr<webrtc::MediaStreamInterface> m_backend;
    GCUnorderedSet<AudioStreamTrack*> m_audioTracks;
    GCUnorderedSet<VideoStreamTrack*> m_videoTracks;

    AudioTrackObserver* m_audioTrackObserver{ nullptr };
    VideoFrameObserver* m_videoFrameObserver{ nullptr };
};
} // namespace Starfish

#endif
#endif
