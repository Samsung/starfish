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

#include "pc/video_track_source.h"
#include "platform/webrtc/VideoCapturer.h"

namespace Starfish {
class ExecutionContext;

class MediaStreamTrack : public EventTarget {
public:
    MediaStreamTrack(ExecutionContext* executionContext);
    virtual ~MediaStreamTrack();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaStreamTrack)
    virtual ExecutionContext* executionContext() const override;

protected:
    ExecutionContext* m_executionContext{ nullptr };
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
    virtual ~VideoStreamTrack();
    virtual ExecutionContext* executionContext() const override;

    CapturerTrackSource* videoDevices()
    {
        return m_videoDevices.get();
    }

private:
    rtc::scoped_refptr<CapturerTrackSource> m_videoDevices;
};

class MediaStream : public EventTarget {
public:
    MediaStream(ExecutionContext* executionContext);
    MediaStream(ExecutionContext* executionContext, MediaStream& mediaStream);
    MediaStream(ExecutionContext* executionContext,
                GCVector<MediaStreamTrack*>& tracks);
    virtual ~MediaStream();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaStream)
    virtual ExecutionContext* executionContext() const override;

private:
    ExecutionContext* m_executionContext{ nullptr };
};
} // namespace Starfish

#endif
#endif
