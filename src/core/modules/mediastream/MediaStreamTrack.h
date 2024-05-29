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

#include "rtc_video_renderer.h"
#include "rtc_video_track.h"
#include "rtc_video_source.h"
#include "rtc_audio_track.h"

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

    virtual String* readyState() = 0;

    void stop();

protected:
    ExecutionContext* m_executionContext{ nullptr };
    WebRtcManager* m_webRtcManager{ nullptr };
    Kind m_kind{ Kind::None };
    GCUnorderedSet<MediaStream*> m_attachedMediaStreams;
};

class AudioStreamTrack : public MediaStreamTrack {
public:
    AudioStreamTrack(ExecutionContext* executionContext);
    AudioStreamTrack(
        ExecutionContext* executionContext,
        libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> backend);
    virtual ~AudioStreamTrack();
    void dispose() override;

    std::string id() override
    {
        STARFISH_UNSUPPORTED("Returning id of AudioStreamTrack");
        return std::string();
    }

    virtual bool isAudioStreamTrack()
    {
        return true;
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> backend()
    {
        return m_backend;
    }

    bool isDisposed()
    {
        return m_backend == nullptr;
    }

    String* readyState() override;

private:
    libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> m_backend;
};

class VideoStreamTrack : public MediaStreamTrack {
public:
    class VideoStreamTrackObserver
        : public gc,
          public libwebrtc::RTCVideoRenderer<
              libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame>> {
    public:
        VideoStreamTrackObserver(libwebrtc::RTCVideoTrack* trackToRender);
        // VideoSinkInterface implementation
        void OnFrame(
            libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame> frame) override;

    private:
        libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> m_back;
    };

    VideoStreamTrack(ExecutionContext* executionContext);
    VideoStreamTrack(
        ExecutionContext* executionContext,
        libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> backend);

    virtual ~VideoStreamTrack();
    void dispose() override;

    std::string id() override
    {
        STARFISH_UNSUPPORTED("Returning id of VideoStreamTrack");
        return std::string();
    }

    virtual bool isVideoStreamTrack() override
    {
        return true;
    }

    virtual libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> backend()
    {
        return m_backend;
    }

    void play();

    bool isDisposed()
    {
        return m_backend == nullptr;
    }

    String* readyState() override;

protected:
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> m_backend;
    VideoStreamTrackObserver* m_source{ nullptr };
};

class WebCamStreamTrack : public VideoStreamTrack {
public:
    WebCamStreamTrack(ExecutionContext* executionContext, size_t width,
                      size_t height, size_t fps);
    WebCamStreamTrack(
        ExecutionContext* executionContext,
        libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> backend);
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
