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

#ifndef __StarfishMediaStreamTrack__
#define __StarfishMediaStreamTrack__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "api/media_stream_interface.h"
#include "pc/video_track_source.h"
#include "platform/webrtc/VideoCapturer.h"

namespace Starfish {
class ExecutionContext;
class AudioStreamTrack;
class VideoStreamTrack;
class WebCamStreamTrack;
class MediaPlayerWebRtc;
class WebRtcManager;

class MediaStreamTrack : public EventTarget {
public:
    String* m_audioTrackLabel = String::createASCIIString("AudioTrack");
    const std::string m_videoTrackLabel = "VideoTrack";

    enum class Kind { Audio, Video, None };

    MediaStreamTrack(ExecutionContext* executionContext);
    virtual ~MediaStreamTrack();
    virtual void dispose() = 0;

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaStreamTrack)
    virtual ExecutionContext* executionContext() const override;

    virtual std::string id() = 0;

    virtual Kind kind()
    {
        return m_kind;
    }

    virtual String* kindString();

    virtual bool isAudioStreamTrack()
    {
        return false;
    }

    virtual bool isVideoStreamTrack()
    {
        return false;
    }

    virtual bool isWebCamStreamTrack()
    {
        return false;
    }

    void attachTo(MediaStream* stream)
    {
        m_attachedMediaStreams.insert(stream);
    }

    void removeFrom(MediaStream* stream)
    {
        m_attachedMediaStreams.erase(stream);
    }

    AudioStreamTrack* asAudioStreamTrack();
    VideoStreamTrack* asVideoStreamTrack();
    WebCamStreamTrack* asWebCamStreamTrack();

protected:
    ExecutionContext* m_executionContext{ nullptr };
    WebRtcManager* m_webRtcManager{ nullptr };
    Kind m_kind{ Kind::None };
    GCUnorderedSet<MediaStream*> m_attachedMediaStreams;
};

class AudioStreamTrack : public MediaStreamTrack {
public:
    AudioStreamTrack(ExecutionContext* executionContext);
    AudioStreamTrack(ExecutionContext* executionContext,
                     rtc::scoped_refptr<webrtc::AudioTrackInterface> backend);
    virtual ~AudioStreamTrack();
    void dispose() override;

    std::string id() override
    {
        return m_backend->id();
    }

    virtual bool isAudioStreamTrack()
    {
        return true;
    }

    rtc::scoped_refptr<webrtc::AudioTrackInterface> backend()
    {
        return m_backend;
    }

private:
    rtc::scoped_refptr<webrtc::AudioTrackInterface> m_backend;
};

class VideoStreamTrack : public MediaStreamTrack {
public:
    class VideoStreamTrackObserver
        : public gc,
          public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    public:
        VideoStreamTrackObserver(webrtc::VideoTrackInterface* trackToRender);
        // VideoSinkInterface implementation
        void OnFrame(const webrtc::VideoFrame& frame) override;

    private:
        rtc::scoped_refptr<webrtc::VideoTrackInterface> m_back;
    };

    VideoStreamTrack(ExecutionContext* executionContext);
    VideoStreamTrack(ExecutionContext* executionContext,
                     rtc::scoped_refptr<webrtc::VideoTrackInterface> backend);

    virtual ~VideoStreamTrack();
    void dispose() override;

    std::string id() override
    {
        return m_backend->id();
    }

    virtual bool isVideoStreamTrack() override
    {
        return true;
    }

    virtual rtc::scoped_refptr<webrtc::VideoTrackInterface> backend()
    {
        return m_backend;
    }

    void play();

protected:
    rtc::scoped_refptr<webrtc::VideoTrackInterface> m_backend;
    VideoStreamTrackObserver* m_source{ nullptr };
};

class WebCamStreamTrack : public VideoStreamTrack {
public:
    class WebCamStreamTrackCapturer : public webrtc::VideoTrackSource {
    public:
        static rtc::scoped_refptr<WebCamStreamTrackCapturer> create(
            size_t width, size_t height, size_t fps);
        void destroy();
        void resetVideoCapturer();

    protected:
        explicit WebCamStreamTrackCapturer(
            std::unique_ptr<VideoCapturer> capturer);

    private:
        rtc::VideoSourceInterface<webrtc::VideoFrame>* source();
        std::unique_ptr<VideoCapturer> m_videoCapturer;
    };

    WebCamStreamTrack(ExecutionContext* executionContext, size_t width,
                      size_t height, size_t fps);
    WebCamStreamTrack(ExecutionContext* executionContext,
                      rtc::scoped_refptr<webrtc::VideoTrackInterface> backend);
    virtual ~WebCamStreamTrack();
    void dispose() override;

    bool isWebCamStreamTrack() override
    {
        return true;
    }
};

class MediaStreamTrackObserver {
public:
    virtual void removeAudioTrack(AudioStreamTrack* track) = 0;
    virtual void removeVideoTrack(VideoStreamTrack* track) = 0;
    virtual ~MediaStreamTrackObserver(){};
};
} // namespace Starfish

#endif
#endif
