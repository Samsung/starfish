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

#include "rtc_peerconnection_factory.h"

namespace Starfish {

MediaStreamTrack::MediaStreamTrack(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((MediaStreamTrack*)obj)->~MediaStreamTrack();
        },
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
        STARFISH_LOG_ERROR("AudioStreamTrack: failed");
    }
}

AudioStreamTrack::AudioStreamTrack(
    ExecutionContext* executionContext,
    libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> backend)
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
    dispose();
}

void AudioStreamTrack::dispose()
{
    for (auto* mediaStream : m_attachedMediaStreams) {
        mediaStream->removeAudioTrack(this);
    }
    m_attachedMediaStreams.clear();

    if (m_backend.get()) {
        m_backend = nullptr;
    }
}

VideoStreamTrack::VideoStreamTrack(ExecutionContext* executionContext)
    : VideoStreamTrack(executionContext, nullptr)
{
}

VideoStreamTrack::VideoStreamTrack(
    ExecutionContext* executionContext,
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> backend)
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
    dispose();
}

void VideoStreamTrack::dispose()
{
    for (auto* mediaStream : m_attachedMediaStreams) {
        mediaStream->removeVideoTrack(this);
    }
    m_attachedMediaStreams.clear();

    if (m_backend.get()) {
        m_backend = nullptr;
    }
}

void VideoStreamTrack::play()
{
    m_source = new VideoStreamTrackObserver(m_backend);
}

VideoStreamTrack::VideoStreamTrackObserver::VideoStreamTrackObserver(
    libwebrtc::RTCVideoTrack* trackToRender)
{
    trackToRender->AddRenderer(this);
}

void VideoStreamTrack::VideoStreamTrackObserver::OnFrame(
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame> frame)
{
}

WebCamStreamTrack::WebCamStreamTrack(ExecutionContext* executionContext,
                                     size_t width_, size_t height_, size_t fps_)
    : WebCamStreamTrack(executionContext, nullptr)
{
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoCapturer> video_capturer;
    char strNameUTF8[256] = {
        0,
    };

    char strGuidUTF8[256] = {
        0,
    };

    int numberOfVideoDevices =
        m_webRtcManager->videoDevice()->NumberOfDevices();

    int32_t width = MediaDevices::kWidth;
    int32_t height = MediaDevices::kHeight;
    int32_t fps = MediaDevices::kFps;

    for (int i = 0; i < numberOfVideoDevices; i++) {
        m_webRtcManager->videoDevice()->GetDeviceName(i, strNameUTF8, 256,
                                                      strGuidUTF8, 256);
        // if (sourceId != "" && sourceId == strGuidUTF8) {
        video_capturer = m_webRtcManager->videoDevice()->Create(
            strNameUTF8, i, width, height, fps);
        break;
        // }
    }

    if (numberOfVideoDevices == 0) {
        return;
    }

    if (!video_capturer.get()) {
        m_webRtcManager->videoDevice()->GetDeviceName(0, strNameUTF8, 128,
                                                      strGuidUTF8, 128);
        video_capturer = m_webRtcManager->videoDevice()->Create(
            strNameUTF8, 0, width, height, fps);
    }

    if (!video_capturer.get()) {
        return;
    }

    const char* videoSourceLabel = "video_input";
    libwebrtc::scoped_refptr<libwebrtc::RTCMediaConstraints> constraints =
        libwebrtc::RTCMediaConstraints::Create();

    libwebrtc::scoped_refptr<libwebrtc::RTCVideoSource> source =
        m_webRtcManager->peerConnectionFactory()->CreateVideoSource(
            video_capturer, videoSourceLabel, constraints);

    m_backend = m_webRtcManager->peerConnectionFactory()->CreateVideoTrack(
        source, m_videoTrackLabel.c_str());

    if (!m_backend.get()) {
        STARFISH_LOG_ERROR("Construction failed");
    }
}

WebCamStreamTrack::WebCamStreamTrack(
    ExecutionContext* executionContext,
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> backend)
    : VideoStreamTrack(executionContext, backend)
{
}

WebCamStreamTrack::~WebCamStreamTrack()
{
    dispose();
}

void WebCamStreamTrack::dispose()
{
    for (auto* mediaStream : m_attachedMediaStreams) {
        mediaStream->removeVideoTrack(this);
    }
    m_attachedMediaStreams.clear();

    if (m_backend.get()) {
        m_backend = nullptr;
    }
}

} // namespace Starfish

#endif
