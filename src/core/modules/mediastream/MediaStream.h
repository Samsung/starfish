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

namespace Starfish {
class ExecutionContext;
class VideoStreamTrack;
class AudioStreamTrack;

class MediaStreamTrack : public EventTarget {
public:
    const std::string m_audioLabel = "audioLabel";
    const std::string m_videoLabel = "videoLabel";

    enum class Kind { Audio, Video, None };

    MediaStreamTrack(ExecutionContext* executionContext);
    virtual ~MediaStreamTrack();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaStreamTrack)
    virtual ExecutionContext* executionContext() const override;

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

protected:
    ExecutionContext* m_executionContext{ nullptr };
    Kind m_kind{ Kind::None };
    GCUnorderedSet<MediaStream*> m_attachedMediaStreams;
};

class AudioStreamTrack : public MediaStreamTrack {
public:
    AudioStreamTrack(ExecutionContext* executionContext);
    AudioStreamTrack(ExecutionContext* executionContext,
                     rtc::scoped_refptr<webrtc::AudioTrackInterface> backend);
    virtual ~AudioStreamTrack();

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
    class CapturerTrackSource : public webrtc::VideoTrackSource {
    public:
        // TODO: get values from user JS script
        static const size_t kWidth = 640;
        static const size_t kHeight = 480;
        static const size_t kFps = 30;

        static rtc::scoped_refptr<CapturerTrackSource> create();

    protected:
        explicit CapturerTrackSource(std::unique_ptr<VideoCapturer> capturer);

    private:
        rtc::VideoSourceInterface<webrtc::VideoFrame>* source();
        std::unique_ptr<VideoCapturer> m_capturer;
    };

    VideoStreamTrack(ExecutionContext* executionContext);
    VideoStreamTrack(ExecutionContext* executionContext,
                     rtc::scoped_refptr<webrtc::VideoTrackInterface> backend);
    virtual ~VideoStreamTrack();

    virtual bool isVideoStreamTrack()
    {
        return true;
    }

    rtc::scoped_refptr<webrtc::VideoTrackInterface> backend()
    {
        return m_backend;
    }

private:
    rtc::scoped_refptr<webrtc::VideoTrackInterface> m_backend;
};

class MediaStreamTrackObserver {
public:
    virtual void removeAudioTrack(AudioStreamTrack* track) = 0;
    virtual void removeVideoTrack(VideoStreamTrack* track) = 0;
};

class MediaStream : public EventTarget, public MediaStreamTrackObserver {
public:
    const std::string m_streamId = "streamId";

    MediaStream(ExecutionContext* executionContext);
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

private:
    ExecutionContext* m_executionContext{ nullptr };
    rtc::scoped_refptr<webrtc::MediaStreamInterface> m_backend;
    GCUnorderedSet<AudioStreamTrack*> m_audioTracks;
    GCUnorderedSet<VideoStreamTrack*> m_videoTracks;
};
} // namespace Starfish

#endif
#endif
