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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/MediaStreamTrack.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/Navigator.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/mediastream/RTCPeerConnection.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

#include "api/peer_connection_interface.h"

namespace Starfish {
MediaStreamTrack::MediaStreamTrack(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((MediaStreamTrack*)obj)->~MediaStreamTrack(); },
        NULL, NULL, NULL);
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
    // TODO: find audio devices for each real target device
    m_backend = m_webRtcManager->createAudioTrack(m_audioTrackLabel);
    if (!m_backend) {
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
    m_webRtcManager = this->executionContext()
                          ->document()
                          ->window()
                          ->navigator()
                          ->webRtcManager();
}

AudioStreamTrack::~AudioStreamTrack()
{
    WEBRTC_LOGI("<%s self=%p>\n", __func__, (void*)this);
    dispose();
    WEBRTC_LOGI("</%s self=%p>\n", __func__, (void*)this);
}

void AudioStreamTrack::dispose()
{
    WEBRTC_LOGI("<AudioStreamTrack::%s self=%p>\n", __func__, (void*)this);
    if (m_webRtcManager->peerConnectionFactory()) {
        m_backend = nullptr;
    } else {
        m_backend.release();
    }
    WEBRTC_LOGI("</AudioStreamTrack::%s self=%p>\n", __func__, (void*)this);
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
    m_webRtcManager = this->executionContext()
                          ->document()
                          ->window()
                          ->navigator()
                          ->webRtcManager();
}

VideoStreamTrack::~VideoStreamTrack()
{
    WEBRTC_LOGI("<VideoStreamTrack::%s self=%p>\n", __func__, (void*)this);
    dispose();
    WEBRTC_LOGI("</VideoStreamTrack::%s self=%p>\n", __func__, (void*)this);
}

void VideoStreamTrack::dispose()
{
    if (m_webRtcManager->peerConnectionFactory()) {
        m_backend = nullptr;
    } else {
        m_backend.release();
    }
    m_attachedMediaStreams.clear();
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
    m_videoDevices = WebCamStreamTrackCapturer::create();
    if (m_videoDevices) {
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
            peerConnectionFactory = this->executionContext()
                                        ->document()
                                        ->window()
                                        ->navigator()
                                        ->webRtcManager()
                                        ->createPeerConnectionFactory();

        STARFISH_ASSERT(peerConnectionFactory);
        m_backend = peerConnectionFactory->CreateVideoTrack(m_videoTrackLabel,
                                                            m_videoDevices);
    } else {
        STARFISH_LOG_ERROR("%s: construction failed\n", __func__);
    }
}

WebCamStreamTrack::WebCamStreamTrack(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::VideoTrackInterface> backend)
    : VideoStreamTrack(executionContext, backend)
{
    WEBRTC_LOGI("<WebCamStreamTrack::%s self=%p>\n", __func__, (void*)this);
    WEBRTC_LOGI("</WebCamStreamTrack::%s self=%p>\n", __func__, (void*)this);
}

WebCamStreamTrack::~WebCamStreamTrack()
{
    WEBRTC_LOGI("<WebCamStreamTrack::%s self=%p>\n", __func__, (void*)this);
    dispose();
    WEBRTC_LOGI("</WebCamStreamTrack::%s self=%p>\n", __func__, (void*)this);
}

void WebCamStreamTrack::dispose()
{
    WEBRTC_LOGI("<WebCamStreamTrack::%s self=%p>\n", __func__, (void*)this);
    if (m_webRtcManager->peerConnectionFactory()) {
        m_backend = nullptr;
    } else {
        m_backend.release();
    }
    m_source = nullptr;
    m_attachedMediaStreams.clear();
    WEBRTC_LOGI("</WebCamStreamTrack::%s self=%p>\n", __func__, (void*)this);
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

void WebCamStreamTrack::WebCamStreamTrackCapturer::destroy()
{
    if (m_videoCapturer) {
        m_videoCapturer->destroy();
    }
    m_videoCapturer = nullptr;
}

void WebCamStreamTrack::WebCamStreamTrackCapturer::resetVideoCapturer()
{
    m_videoCapturer = nullptr;
}

rtc::VideoSourceInterface<webrtc::VideoFrame>*
WebCamStreamTrack::WebCamStreamTrackCapturer::source()
{
    return m_videoCapturer.get();
}
}
#endif
