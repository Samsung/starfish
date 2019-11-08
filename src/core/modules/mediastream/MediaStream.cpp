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

#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "api/video/video_source_interface.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"

#include "platform/multimedia/MediaPlayerWebRtc.h"

namespace Starfish {

MediaStream::MediaStreamObserver::MediaStreamObserver(
    webrtc::VideoTrackInterface* trackToRender, MediaPlayerWebRtc* player)
    : m_trackToRender(trackToRender)
{
    m_trackToRender->AddOrUpdateSink(this, rtc::VideoSinkWants());
    m_player = player;

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((MediaStreamObserver*)obj)->~MediaStreamObserver();
        },
        NULL, NULL, NULL);
}

MediaStream::MediaStreamObserver::~MediaStreamObserver()
{
    m_trackToRender->RemoveSink(this);
}

void MediaStream::MediaStreamObserver::setSize(int width, int height)
{
    if (m_width == width && m_height == height) {
        return;
    }

    m_width = width;
    m_height = height;
    m_image.reset(new uint8_t[width * height * 4]);
}

void MediaStream::MediaStreamObserver::OnFrame(
    const webrtc::VideoFrame& videoFrame)
{
    // TODO: Consider having a thread after measuring the performance
    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
        videoFrame.video_frame_buffer()->ToI420());
    if (videoFrame.rotation() != webrtc::kVideoRotation_0) {
        buffer = webrtc::I420Buffer::Rotate(*buffer, videoFrame.rotation());
    }
    setSize(buffer->width(), buffer->height());

    // Due to a bug (https://bugs.webrtc.org/6857), libyuv::I420ToRGBA()
    // generates a red video output.
    // I420ToABGR generates [(r,g,b,a)]
    // I420ToARGB generates [(b,g,r,a)]
    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                       m_image.get(), m_width * 4, buffer->width(),
                       buffer->height());

    m_player->onFrame(m_image.get());
}

MediaStream::MediaStream(ExecutionContext* executionContext)
    : MediaStream(executionContext, nullptr)
{
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        peerConnectionFactory = this->executionContext()
                                    ->document()
                                    ->window()
                                    ->navigator()
                                    ->webRtcManager()
                                    ->peerConnectionFactory();
    STARFISH_ASSERT(peerConnectionFactory);

    char streamId[100];
    snprintf(streamId, sizeof(streamId), "%s:%p", m_mediaStreamLabel.c_str(),
             (void*)this);
    m_backend = peerConnectionFactory->CreateLocalMediaStream(streamId);
}

MediaStream::MediaStream(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::MediaStreamInterface> backend)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_backend(backend)
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
    STARFISH_LOG_INFO("%s\n", __func__);
    m_backend = nullptr;
    m_mediaStreamObserver = nullptr;
    m_audioTracks.clear();
    m_videoTracks.clear();
}

ScriptBindingInstance* MediaStream::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* MediaStream::executionContext() const
{
    return m_executionContext;
}

GCVector<MediaStreamTrack*> MediaStream::getAudioTracks()
{
    GCVector<MediaStreamTrack*> tracks;
    tracks.insert(tracks.end(), m_audioTracks.begin(), m_audioTracks.end());
    return tracks;
}

GCVector<MediaStreamTrack*> MediaStream::getVideoTracks()
{
    GCVector<MediaStreamTrack*> tracks;
    tracks.insert(tracks.end(), m_videoTracks.begin(), m_videoTracks.end());
    return tracks;
}

GCVector<MediaStreamTrack*> MediaStream::getTracks()
{
    GCVector<MediaStreamTrack*> tracks;
    tracks.insert(tracks.end(), m_audioTracks.begin(), m_audioTracks.end());
    tracks.insert(tracks.end(), m_videoTracks.begin(), m_videoTracks.end());
    return tracks;
}

void MediaStream::addTrack(MediaStreamTrack* track)
{
    if (track == nullptr) {
        return;
    }

    if (track->kind() == MediaStreamTrack::Kind::Audio) {
        auto audioTrack = static_cast<AudioStreamTrack*>(track);
        if (audioTrack->backend()) {
            m_backend->AddTrack(audioTrack->backend());
            m_audioTracks.insert(audioTrack);
            audioTrack->attachTo(this);
        }
    } else if (track->kind() == MediaStreamTrack::Kind::Video) {
        auto videoTrack = track->asVideoStreamTrack();
        if (videoTrack->backend()) {
            m_backend->AddTrack(videoTrack->backend());
            m_videoTracks.insert(videoTrack);
            videoTrack->attachTo(this);
        }
    }
}

void MediaStream::removeTrack(MediaStreamTrack* track)
{
    if (track == nullptr) {
        return;
    }

    if (track->kind() == MediaStreamTrack::Kind::Audio) {
        auto audioTrack = static_cast<AudioStreamTrack*>(track);
        removeAudioTrack(audioTrack);
        audioTrack->removeFrom(this);
    } else if (track->kind() == MediaStreamTrack::Kind::Video) {
        auto videoTrack = static_cast<WebCamStreamTrack*>(track);
        removeVideoTrack(videoTrack);
        videoTrack->removeFrom(this);
    }
}

void MediaStream::removeAudioTrack(AudioStreamTrack* track)
{
    STARFISH_ASSERT(track);

    if (track->backend()) {
        m_backend->RemoveTrack(track->backend());
    }
    m_audioTracks.erase(track);
}

void MediaStream::removeVideoTrack(VideoStreamTrack* track)
{
    STARFISH_ASSERT(track);

    if (track->backend()) {
        m_backend->RemoveTrack(track->backend());
    }
    m_videoTracks.erase(track);
}

void MediaStream::startPlayVideoTrack(MediaPlayerWebRtc* player,
                                      MediaStreamTrack* track)
{
    STARFISH_ASSERT(track);

    if (track->isVideoStreamTrack()) {
        VideoStreamTrack* videoTrack = track->asVideoStreamTrack();

        if (videoTrack->backend()) {
            m_mediaStreamObserver.reset(
                new MediaStreamObserver(videoTrack->backend(), player));
        } else {
            STARFISH_LOG_WARN("%s: backend() == nullptr\n", __func__);
        }
    }
}

void MediaStream::stopPlayVideoTrack()
{
    m_mediaStreamObserver == nullptr;
}
}
#endif
