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
#include "core/modules/mediastream/RTCPeerConnection.h"

namespace Starfish {
class ExecutionContext;
class MediaPlayerWebRtc;
class Mutex;
class WebRtcManager;

class MediaStream : public EventTarget, public MediaStreamTrackObserver {
public:
    static const int PIXEL_STRIDE = 4;

    class VideoFrameObserver
        : public gc,
          public rtc::VideoSinkInterface<webrtc::VideoFrame> {
        friend MediaStream;

    public:
        VideoFrameObserver(MediaStream* mediaStream,
                           webrtc::VideoTrackInterface* videoTrack);
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

        Mutex* imageLock()
        {
            return m_imageLock;
        }

    private:
        void setSize(int width, int height);
#if defined(STARFISH_WEBRTC_DEBUG)
        void writeImageToFile(std::string& filename);
#endif
        MediaStream* m_mediaStream{ nullptr };
        rtc::scoped_refptr<webrtc::VideoTrackInterface> m_videoTrack;

        std::unique_ptr<uint8_t[]> m_image;
        Mutex* m_imageLock{ nullptr };
        int m_width{ 0 };
        int m_height{ 0 };
    };

    class AudioTrackObserver : public gc,
                               public webrtc::AudioTrackSinkInterface {
        friend MediaStream;

    public:
        AudioTrackObserver(MediaStream* mediaStream,
                           webrtc::AudioTrackInterface* audioTrack);
        virtual ~AudioTrackObserver();

        // AudioAudioTrackSinkInterface implementation
        void OnData(const void* audioData, int bitsPerSample, int sampleRate,
                    size_t numberOfChannels, size_t numberOfFrames) override;

        int16_t* audioData()
        {
            return m_audioData.get();
        }

        int numberOfFrames()
        {
            return m_numberOfFrames;
        }

        void setSize(int size);
        void stop();

        Mutex* audioLock()
        {
            return m_audioLock;
        }

    private:
        MediaStream* m_mediaStream{ nullptr };
        rtc::scoped_refptr<webrtc::AudioTrackInterface> m_audioTrack;

        std::unique_ptr<int16_t[]> m_audioData;
        Mutex* m_audioLock{ nullptr };

        int m_bitsPerSample{ 0 };
        int m_sampleRate{ 0 };
        int m_numberOfChannels{ 0 };
        int m_numberOfFrames{ 0 };
    };

    const std::string m_mediaStreamLabel = "MediaStream";

    MediaStream(ExecutionContext* executionContext);
    MediaStream(ExecutionContext* executionContext,
                rtc::scoped_refptr<webrtc::MediaStreamInterface> backend);
    MediaStream(ExecutionContext* executionContext, MediaStream& mediaStream);
    MediaStream(ExecutionContext* executionContext,
                GCVector<MediaStreamTrack*>& tracks);
    virtual ~MediaStream();
    void dispose();

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

    void playAudioTrack(MediaStreamTrack* track);
    void playVideoTrack(MediaStreamTrack* track);

    void stopAudioTrack();
    void stopVideoTrack();

    void syncTracks();

    rtc::scoped_refptr<webrtc::MediaStreamInterface> backend()
    {
        return m_backend;
    }

    void setMediaPlayer(MediaPlayerWebRtc* mediaPlayer)
    {
        m_mediaPlayer = mediaPlayer;
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    WebRtcManager* m_webRtcManager{ nullptr };
    rtc::scoped_refptr<webrtc::MediaStreamInterface> m_backend;
    GCUnorderedSet<AudioStreamTrack*> m_audioTracks;
    GCUnorderedSet<VideoStreamTrack*> m_videoTracks;

    AudioTrackObserver* m_audioTrackObserver{ nullptr };
    VideoFrameObserver* m_videoFrameObserver{ nullptr };
    MediaPlayerWebRtc* m_mediaPlayer{ nullptr };
};
} // namespace Starfish

#endif
#endif
