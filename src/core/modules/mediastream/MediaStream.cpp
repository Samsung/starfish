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

#include "core/modules/mediastream/MediaStream.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/Navigator.h"
#include "core/page/WebBase.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"

#include "platform/multimedia/MediaPlayerWebRtc.h"

#include "core/page/GlobalScope.h"

namespace Starfish {

MediaStream::AudioTrackObserver::AudioTrackObserver(
    MediaStream* mediaStream, libwebrtc::RTCAudioTrack* audioTrack)
    : m_mediaStream(mediaStream)
    , m_audioTrack(audioTrack)
    , m_audioLock(new Mutex())
{
    // if (m_audioTrack) {
    //     m_audioTrack->AddSink(this);
    // }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((AudioTrackObserver*)obj)->~AudioTrackObserver();
        },
        NULL, NULL, NULL);
}

MediaStream::AudioTrackObserver::~AudioTrackObserver()
{
    stop();
    m_mediaStream = nullptr;
    m_audioTrack = nullptr;
}

void MediaStream::AudioTrackObserver::setSize(int size)
{
    STARFISH_UNIMPLEMENTED();
    // if (m_numberOfFrames == size) {
    //     return;
    // }

    // m_audioData.reset(new int16_t[size]);
}

void MediaStream::AudioTrackObserver::UpdateFrame(int id, uint32_t timestamp,
                                                  const int16_t* data,
                                                  size_t samples_per_channel,
                                                  int sample_rate_hz,
                                                  size_t num_channels)
{
    // ?? setSize(numberOfFrames);

    m_id = id;
    m_timestamp = timestamp;
    m_samplesPerChannel = samples_per_channel;
    m_sampleRateHz = sample_rate_hz;
    m_numChannels = num_channels;

    if (m_mediaStream && m_mediaStream->m_mediaPlayer) {
        // m_mediaStream->m_mediaPlayer->onData(this);
    }
}

void MediaStream::AudioTrackObserver::CopyFrom(const AudioFrame& src)
{
    STARFISH_UNIMPLEMENTED();
}

void MediaStream::AudioTrackObserver::Add(const AudioFrame& frameToAdd)
{
    STARFISH_UNIMPLEMENTED();
}

void MediaStream::AudioTrackObserver::Mute()
{
    STARFISH_UNIMPLEMENTED();
}

void MediaStream::AudioTrackObserver::stop()
{
    // if (m_audioTrack) {
    //     m_audioTrack->RemoveSink(this);
    // }

    if (m_mediaStream &&
        !m_mediaStream->m_webRtcManager->peerConnectionFactory()) {
        m_audioTrack.release();
    } else {
        m_audioTrack = nullptr;
    }
}

MediaStream::VideoFrameObserver::VideoFrameObserver(
    MediaStream* mediaStream, libwebrtc::RTCVideoTrack* videoTrack)
    : m_mediaStream(mediaStream)
    , m_videoTrack(videoTrack)
    , m_imageLock(new Mutex())
{
    if (m_videoTrack) {
        // m_videoTrack->RemoveRenderer(this);
        m_videoTrack->AddRenderer(this);
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((VideoFrameObserver*)obj)->~VideoFrameObserver();
        },
        NULL, NULL, NULL);
}

MediaStream::VideoFrameObserver::~VideoFrameObserver()
{
    stop();
    m_mediaStream = nullptr;
    m_videoTrack = nullptr;
}

void MediaStream::VideoFrameObserver::setSize(int width, int height)
{
    if (m_width == width && m_height == height) {
        return;
    }

    m_width = width;
    m_height = height;
    m_image.reset(new uint8_t[width * height * pixelStride()]);
}

void MediaStream::VideoFrameObserver::OnFrame(
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame> frame)
{
    {
        Locker<Mutex> lock(*imageLock());
        setSize(frame->width(), frame->height());
        frame->ConvertToARGB(libwebrtc::RTCVideoFrame::Type::kARGB,
                             m_image.get(), 0, m_width, m_height);
    }

    if (m_mediaStream && m_mediaStream->m_mediaPlayer) {
        m_mediaStream->m_mediaPlayer->onFrame(this);
    }
}

void MediaStream::VideoFrameObserver::stop()
{
    if (m_videoTrack) {
        m_videoTrack->RemoveRenderer(this);
        m_image.reset();
    }

    if (m_mediaStream &&
        !m_mediaStream->m_webRtcManager->peerConnectionFactory()) {
        m_videoTrack.release();
    } else {
        m_videoTrack = nullptr;
    }
}

#if 0 && defined(STARFISH_WEBRTC_DEBUG)
void MediaStream::VideoFrameObserver::writeImageToFile(std::string& filename)
{
    FILE* imageFile = fopen(filename.data(), "wb");
    if (imageFile == nullptr) {
        STARFISH_LOG_WARN("ERROR: Cannot open output file");
        return;
    }

    fprintf(imageFile, "P3\n");                       // P3 filetype
    fprintf(imageFile, "%d %d\n", m_width, m_height); // dimensions
    fprintf(imageFile, "255\n");                      // Max pixel

    int pos = 0;
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            fprintf(imageFile, "%d ", m_image[pos + 0]); // r
            fprintf(imageFile, "%d ", m_image[pos + 1]); // g
            fprintf(imageFile, "%d ", m_image[pos + 2]); // b
            pos += 4;                                    // skip a
        }
        fprintf(imageFile, "\n");
    }
    fprintf(imageFile, "\n");
    fclose(imageFile);
}
#endif

MediaStream::MediaStream(ExecutionContext* executionContext)
    : MediaStream(executionContext, nullptr)
{
    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnectionFactory>
        peerConnectionFactory = m_webRtcManager->peerConnectionFactory();
    STARFISH_ASSERT(peerConnectionFactory);

    char streamId[100];
    snprintf(streamId, sizeof(streamId), "%s:%p", m_mediaStreamLabel.c_str(),
             (void*)this);
    m_backend = peerConnectionFactory->CreateStream(streamId);
}

MediaStream::MediaStream(
    ExecutionContext* executionContext,
    libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream> backend)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_backend(backend)
{
    STARFISH_LOG_INFO("<MediaStream self=%p>", this);

    if (backend) {
        syncTracks();
    }

    m_webRtcManager = this->executionContext()
                          ->document()
                          ->window()
                          ->navigator()
                          ->webRtcManager();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((MediaStream*)obj)->~MediaStream(); },
        NULL, NULL, NULL);

    STARFISH_LOG_INFO("</MediaStream self=%p>", (void*)this);
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
    STARFISH_LOG_INFO("<MediaStream self=%p>", this);
    dispose();
    STARFISH_LOG_INFO("</MediaStream self=%p>", this);
}

void MediaStream::dispose()
{
    STARFISH_LOG_INFO("<MediaStream self=%p>", this);

    stopAudioTrack();
    stopVideoTrack();
    if (m_audioTrackObserver) {
        m_audioTrackObserver->m_mediaStream = nullptr;
        m_audioTrackObserver = nullptr;
    }
    if (m_videoFrameObserver) {
        m_videoFrameObserver->m_mediaStream = nullptr;
        m_videoFrameObserver = nullptr;
    }
    m_mediaPlayer = nullptr;

    if (m_webRtcManager->peerConnectionFactory()) {
        m_backend = nullptr;
    } else {
        m_backend.release();
    }

    for (auto audioTrack : m_audioTracks) {
        audioTrack->dispose();
    }
    m_audioTracks.clear();

    for (auto videoTrack : m_videoTracks) {
        videoTrack->dispose();
    }
    m_videoTracks.clear();
    STARFISH_LOG_INFO("</MediaStream self=%p>", this);
}

ScriptBindingInstance* MediaStream::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* MediaStream::executionContext() const
{
    return m_executionContext;
}

String* MediaStream::id()
{
    std::string id = m_backend->id().std_string();
    return String::createASCIIString(id.data(), id.length());
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
        auto audioTrack = track->asAudioStreamTrack();
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

void MediaStream::playAudioTrack(MediaStreamTrack* track)
{
    STARFISH_ASSERT(track);

    if (track->isAudioStreamTrack()) {
        AudioStreamTrack* audioTrack = track->asAudioStreamTrack();
        if (audioTrack->backend()) {
            m_audioTrackObserver =
                new AudioTrackObserver(this, audioTrack->backend());
        } else {
            STARFISH_LOG_WARN("backend() == nullptr");
        }
    }
}

void MediaStream::playVideoTrack(MediaStreamTrack* track)
{
    STARFISH_ASSERT(track);

    if (track->isVideoStreamTrack()) {
        VideoStreamTrack* videoTrack = track->asVideoStreamTrack();
        if (videoTrack->backend()) {
            m_videoFrameObserver =
                new VideoFrameObserver(this, videoTrack->backend());
        } else {
            STARFISH_LOG_WARN("backend() == nullptr");
        }
    }
}

void MediaStream::stopAudioTrack()
{
    if (m_audioTrackObserver) {
        m_audioTrackObserver->stop();
    }
}

void MediaStream::stopVideoTrack()
{
    if (m_videoFrameObserver) {
        m_videoFrameObserver->stop();
    }
}

void MediaStream::syncTracks()
{
    {
        GCUnorderedMap<libwebrtc::RTCAudioTrack*, AudioStreamTrack*>
            curAudioTracks;
        for (auto audioTrack : m_audioTracks) {
            curAudioTracks.insert(
                std::make_pair(audioTrack->backend().get(), audioTrack));
        }
        for (auto track : m_audioTracks) {
            track->dispose();
        }
        m_audioTracks.clear();

        std::vector<libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack>>
            backendAudioTracks = m_backend->audio_tracks().std_vector();
        for (auto audioTrack : backendAudioTracks) {
            auto itr = curAudioTracks.find(audioTrack.get());
            if (itr != curAudioTracks.end()) {
                m_audioTracks.insert(itr->second);
            } else {
                AudioStreamTrack* newAudioTrack =
                    new AudioStreamTrack(executionContext(), audioTrack);
                addTrack(newAudioTrack);
            }
        }
    }

    {
        GCUnorderedMap<libwebrtc::RTCVideoTrack*, VideoStreamTrack*>
            curVideoTracks;
        for (auto videoTrack : m_videoTracks) {
            curVideoTracks.insert(
                std::make_pair(videoTrack->backend().get(), videoTrack));
        }
        for (auto track : m_videoTracks) {
            track->dispose();
        }
        m_videoTracks.clear();

        std::vector<libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack>>
            backendVideoTracks = m_backend->video_tracks().std_vector();
        for (auto videoTrack : backendVideoTracks) {
            auto itr = curVideoTracks.find(videoTrack.get());
            if (itr != curVideoTracks.end()) {
                m_videoTracks.insert(itr->second);
            } else {
                VideoStreamTrack* newVideoTrack =
                    new VideoStreamTrack(executionContext(), videoTrack);
                addTrack(newVideoTrack);
            }
        }
    }
}
} // namespace Starfish
#endif
