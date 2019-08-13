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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/MediaStream.h"

#include "core/dom/ExecutionContext.h"

#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

namespace Starfish {

MediaStreamTrack::MediaStreamTrack(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
}

MediaStreamTrack::~MediaStreamTrack()
{
}

ScriptBindingInstance* MediaStreamTrack::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* MediaStreamTrack::executionContext() const
{
    return m_executionContext;
}

VideoStreamTrack::VideoStreamTrack(ExecutionContext* executionContext)
    : MediaStreamTrack(executionContext)
{
    m_videoDevices = CapturerTrackSource::create();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((VideoStreamTrack*)obj)->~VideoStreamTrack(); },
        NULL, NULL, NULL);
}

VideoStreamTrack::~VideoStreamTrack()
{
}

ExecutionContext* VideoStreamTrack::executionContext() const
{
    return m_executionContext;
}

rtc::scoped_refptr<VideoStreamTrack::CapturerTrackSource>
VideoStreamTrack::CapturerTrackSource::create()
{
    std::unique_ptr<VideoCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
        return nullptr;
    }
    int numDevices = info->NumberOfDevices();
    for (int i = 0; i < numDevices; ++i) {
        capturer =
            absl::WrapUnique(VideoCapturer::create(kWidth, kHeight, kFps, i));
        if (capturer) {
            return new rtc::RefCountedObject<CapturerTrackSource>(
                std::move(capturer));
        }
    }

    return nullptr;
}

VideoStreamTrack::CapturerTrackSource::CapturerTrackSource(
    std::unique_ptr<VideoCapturer> capturer)
    : VideoTrackSource(/*remote=*/false)
    , m_capturer(std::move(capturer))
{
}

rtc::VideoSourceInterface<webrtc::VideoFrame>*
VideoStreamTrack::CapturerTrackSource::source()
{
    return m_capturer.get();
}

MediaStream::MediaStream(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((MediaStream*)obj)->~MediaStream(); },
        NULL, NULL, NULL);
}

MediaStream::MediaStream(ExecutionContext* executionContext,
                         MediaStream& mediaStream)
    : MediaStream(executionContext)
{
}

MediaStream::MediaStream(ExecutionContext* executionContext,
                         GCVector<MediaStreamTrack*>& tracks)
    : MediaStream(executionContext)
{
}

MediaStream::~MediaStream()
{
}

ScriptBindingInstance* MediaStream::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* MediaStream::executionContext() const
{
    return m_executionContext;
}
}

#endif
