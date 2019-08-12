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
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/Navigator.h"
#include "core/modules/mediastream/WebRtcManager.h"
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

String* MediaStreamTrack::kindString()
{
    switch (m_kind) {
    case Kind::Audio:
        return String::createASCIIString("audio");
    case Kind::Video:
        return String::createASCIIString("video");
    default:
        return String::emptyString;
    }
}

VideoStreamTrack* MediaStreamTrack::asVideoStreamTrack()
{
    STARFISH_ASSERT(isVideoStreamTrack());
    return static_cast<VideoStreamTrack*>(this);
}

VideoStreamTrack::VideoStreamTrack(ExecutionContext* executionContext)
    : VideoStreamTrack(executionContext, nullptr)
{
    rtc::scoped_refptr<CapturerTrackSource> videoDevices =
        CapturerTrackSource::create();
    if (videoDevices) {
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
            peerConnectionFactory = this->executionContext()
                                        ->document()
                                        ->window()
                                        ->navigator()
                                        ->webRtcManager()
                                        ->peerConnectionFactory();

        STARFISH_ASSERT(peerConnectionFactory);
        m_backend =
            peerConnectionFactory->CreateVideoTrack(m_videoLabel, videoDevices);
    }
}

VideoStreamTrack::VideoStreamTrack(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::VideoTrackInterface> backend)
    : MediaStreamTrack(executionContext)
{
    m_kind = Kind::Video;
    m_backend = backend;

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((VideoStreamTrack*)obj)->~VideoStreamTrack(); },
        NULL, NULL, NULL);
}

VideoStreamTrack::~VideoStreamTrack()
{
    auto pc = this->executionContext()
                  ->document()
                  ->window()
                  ->navigator()
                  ->webRtcManager()
                  ->peerConnection();

    if (pc) {
        for (auto& transceiver : pc->GetTransceivers()) {
            pc->RemoveTrack(transceiver->sender());
        }
    }
    m_backend = nullptr;
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
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        peerConnectionFactory = this->executionContext()
                                    ->document()
                                    ->window()
                                    ->navigator()
                                    ->webRtcManager()
                                    ->peerConnectionFactory();
    STARFISH_ASSERT(peerConnectionFactory);
    m_backend = peerConnectionFactory->CreateLocalMediaStream(m_streamId);

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
    auto pc = this->executionContext()
                  ->document()
                  ->window()
                  ->navigator()
                  ->webRtcManager()
                  ->peerConnection();

    if (pc && m_backend) {
        pc->RemoveStream(m_backend);
    }
    m_backend = nullptr;
}

ScriptBindingInstance* MediaStream::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* MediaStream::executionContext() const
{
    return m_executionContext;
}

GCVector<MediaStreamTrack*> MediaStream::getVideoTracks()
{
    GCVector<MediaStreamTrack*> tracks;
    for (auto track : m_backend->GetVideoTracks()) {
        tracks.push_back(new VideoStreamTrack(executionContext(), track));
    }

    return tracks;
}

GCVector<MediaStreamTrack*> MediaStream::getTracks()
{
    // TODO: Support audio tracks
    return getVideoTracks();
}

void MediaStream::addTrack(MediaStreamTrack* track)
{
    if (track->kind() == MediaStreamTrack::Kind::Video) {
        auto videoTrack = static_cast<VideoStreamTrack*>(track);
        if (videoTrack->backend()) {
            m_backend->AddTrack(videoTrack->backend());
        }
    }
}
}

#endif
