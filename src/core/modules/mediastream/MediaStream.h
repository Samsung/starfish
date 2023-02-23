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

#ifndef __StarfishMediaStream__
#define __StarfishMediaStream__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "rtc_media_stream.h"
#include "rtc_video_track.h"
#include "rtc_video_frame.h"
#include "rtc_audio_frame.h"

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
          public libwebrtc::RTCVideoRenderer<
              libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame>> {
        friend MediaStream;

    public:
        VideoFrameObserver(MediaStream* mediaStream,
                           libwebrtc::RTCVideoTrack* videoTrack);
        virtual ~VideoFrameObserver();

        // VideoSinkInterface implementation
        void OnFrame(
            libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame> frame) override;

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
        libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> m_videoTrack;

        std::unique_ptr<uint8_t[]> m_image;
        Mutex* m_imageLock{ nullptr };
        int m_width{ 0 };
        int m_height{ 0 };
    };

    class AudioTrackObserver : public gc, public b2bua::AudioFrame {
        friend MediaStream;

    public:
        AudioTrackObserver(MediaStream* mediaStream,
                           libwebrtc::RTCAudioTrack* audioTrack);
        virtual ~AudioTrackObserver();

        virtual void Release() override{};

        virtual void UpdateFrame(int id, uint32_t timestamp,
                                 const int16_t* data, size_t samplesPerChannel,
                                 int sampleRateHz,
                                 size_t numChannels = 1) override;

        virtual void CopyFrom(const AudioFrame& src) override;

        virtual void Add(const AudioFrame& frameToAdd) override;

        virtual void Mute() override;

        virtual const int16_t* data() override
        {
            return m_audioData.get();
        }

        virtual size_t samples_per_channel() override
        {
            return m_samplesPerChannel;
        }

        virtual int sample_rate_hz() override
        {
            return m_sampleRateHz;
        }

        virtual size_t num_channels() override
        {
            return m_numChannels;
        }

        virtual uint32_t timestamp() override
        {
            return m_timestamp;
        }

        virtual int id() override
        {
            return m_id;
        }

        void setSize(int size);
        void stop();

        Mutex* audioLock()
        {
            return m_audioLock;
        }

    private:
        MediaStream* m_mediaStream{ nullptr };
        libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> m_audioTrack;

        std::unique_ptr<int16_t[]> m_audioData;
        Mutex* m_audioLock{ nullptr };

        int m_samplesPerChannel{ 0 };
        int m_sampleRateHz{ 0 };
        int m_numChannels{ 0 };
        int m_id{ 0 };
        uint32_t m_timestamp{ 0 };
    };

    const std::string m_mediaStreamLabel = "MediaStream";

    MediaStream(ExecutionContext* executionContext);
    MediaStream(ExecutionContext* executionContext,
                libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream> backend);
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

    libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream> backend()
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
    libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream> m_backend;
    GCUnorderedSet<AudioStreamTrack*> m_audioTracks;
    GCUnorderedSet<VideoStreamTrack*> m_videoTracks;

    AudioTrackObserver* m_audioTrackObserver{ nullptr };
    VideoFrameObserver* m_videoFrameObserver{ nullptr };
    MediaPlayerWebRtc* m_mediaPlayer{ nullptr };
};
} // namespace Starfish

#endif
#endif
