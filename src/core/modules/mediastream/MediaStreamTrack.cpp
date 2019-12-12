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

#include "core/modules/mediastream/MediaStreamTrack.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/Navigator.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

#include "api/peer_connection_interface.h"

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

AudioStreamTrack* MediaStreamTrack::asAudioStreamTrack()
{
    STARFISH_ASSERT(isAudioStreamTrack());
    return static_cast<AudioStreamTrack*>(this);
}

VideoStreamTrack* MediaStreamTrack::asVideoStreamTrack()
{
    STARFISH_ASSERT(isVideoStreamTrack());
    return static_cast<VideoStreamTrack*>(this);
}

WebCamStreamTrack* MediaStreamTrack::asWebCamStreamTrack()
{
    STARFISH_ASSERT(isWebCamStreamTrack());
    return static_cast<WebCamStreamTrack*>(this);
}

AudioStreamTrack::AudioStreamTrack(ExecutionContext* executionContext)
    : AudioStreamTrack(executionContext, nullptr)
{
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        peerConnectionFactory = this->executionContext()
                                    ->document()
                                    ->window()
                                    ->navigator()
                                    ->webRtcManager()
                                    ->peerConnectionFactory();

    STARFISH_ASSERT(peerConnectionFactory);

    // TODO: find audio devices for each real target device
    rtc::scoped_refptr<webrtc::AudioSourceInterface> audioDevice =
        peerConnectionFactory->CreateAudioSource(cricket::AudioOptions());
    if (audioDevice) {
        m_backend = peerConnectionFactory->CreateAudioTrack(m_audioTrackLabel,
                                                            audioDevice);
    } else {
        STARFISH_LOG_ERROR("AudioStreamTrack: failed\n");
    }
}

AudioStreamTrack::AudioStreamTrack(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::AudioTrackInterface> backend)
    : MediaStreamTrack(executionContext)
{
    m_kind = Kind::Audio;
    m_backend = backend;

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((AudioStreamTrack*)obj)->~AudioStreamTrack(); },
        NULL, NULL, NULL);
}

AudioStreamTrack::~AudioStreamTrack()
{
    STARFISH_LOG_INFO("%s\n", __func__);
    m_backend = nullptr;
}

VideoStreamTrack::VideoStreamTrack(ExecutionContext* executionContext)
    : VideoStreamTrack(executionContext, nullptr)
{
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
    STARFISH_LOG_INFO("%s\n", __func__);
    m_backend = nullptr;
}

void VideoStreamTrack::play()
{
    m_source = new VideoStreamTrackObserver(m_backend);
}

VideoStreamTrack::VideoStreamTrackObserver::VideoStreamTrackObserver(
    webrtc::VideoTrackInterface* trackToRender)
{
    trackToRender->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

void VideoStreamTrack::VideoStreamTrackObserver::OnFrame(
    const webrtc::VideoFrame& frame)
{
}

WebCamStreamTrack::WebCamStreamTrack(ExecutionContext* executionContext)
    : WebCamStreamTrack(executionContext, nullptr)
{
    rtc::scoped_refptr<WebCamStreamTrackCapturer> videoDevices =
        WebCamStreamTrackCapturer::create();
    if (videoDevices) {
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
            peerConnectionFactory = this->executionContext()
                                        ->document()
                                        ->window()
                                        ->navigator()
                                        ->webRtcManager()
                                        ->peerConnectionFactory();

        STARFISH_ASSERT(peerConnectionFactory);
        m_backend = peerConnectionFactory->CreateVideoTrack(m_videoTrackLabel,
                                                            videoDevices);
    } else {
        STARFISH_LOG_ERROR("%s: construction failed\n", __func__);
    }
}

WebCamStreamTrack::WebCamStreamTrack(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::VideoTrackInterface> backend)
    : VideoStreamTrack(executionContext, backend)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((WebCamStreamTrack*)obj)->~WebCamStreamTrack(); },
        NULL, NULL, NULL);
}

WebCamStreamTrack::~WebCamStreamTrack()
{
    STARFISH_LOG_INFO("%s\n", __func__);
    m_backend = nullptr;
}

rtc::scoped_refptr<WebCamStreamTrack::WebCamStreamTrackCapturer>
WebCamStreamTrack::WebCamStreamTrackCapturer::create()
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
            return new rtc::RefCountedObject<WebCamStreamTrackCapturer>(
                std::move(capturer));
        }
    }

    return nullptr;
}

WebCamStreamTrack::WebCamStreamTrackCapturer::WebCamStreamTrackCapturer(
    std::unique_ptr<VideoCapturer> capturer)
    : VideoTrackSource(/*remote=*/false)
    , m_videoCapturer(std::move(capturer))
{
}

rtc::VideoSourceInterface<webrtc::VideoFrame>*
WebCamStreamTrack::WebCamStreamTrackCapturer::source()
{
    return m_videoCapturer.get();
}
}
#endif
