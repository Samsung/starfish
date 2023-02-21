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

#include "core/modules/mediastream/RTCRtpReceiver.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/MediaStreamTrack.h"
#include "core/modules/mediastream/RTCRtpTransceiver.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/page/Navigator.h"
#include "core/page/Window.h"

namespace Starfish {
RTCRtpReceiver::RTCRtpReceiver(
    ExecutionContext* executionContext, RTCRtpTransceiver* transceiver,
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpReceiver> rtpReceiver)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_transceiver(transceiver)
    , m_backend(rtpReceiver)
{
    syncStreams();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCRtpReceiver*)obj)->~RTCRtpReceiver(); },
        NULL, NULL, NULL);
}

RTCRtpReceiver::~RTCRtpReceiver()
{
    dispose();
}

void RTCRtpReceiver::dispose()
{
    WebRtcManager* webRtcManager = this->m_executionContext->document()
                                       ->window()
                                       ->navigator()
                                       ->webRtcManager();
    if (webRtcManager->peerConnectionFactory()) {
        m_backend = nullptr;
    } else {
        m_backend.release();
    }
    for (auto stream : m_streams) {
        stream->dispose();
    }
    m_streams.clear();
}

ScriptBindingInstance* RTCRtpReceiver::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

MediaStreamTrack* RTCRtpReceiver::track()
{
    if (m_track) {
        if (m_track->isAudioStreamTrack()) {
            STARFISH_RELEASE_ASSERT(
                m_track->asAudioStreamTrack()->backend().get() ==
                m_backend->track().get());
        } else {
            STARFISH_RELEASE_ASSERT(
                m_track->asVideoStreamTrack()->backend().get() ==
                m_backend->track().get());
        }

        return m_track;
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCMediaTrack> mediaTrack =
        m_backend->track();

    if (!mediaTrack.get()) {
        return nullptr;
    }

    if (mediaTrack->kind().std_string() == "audio") {
        libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> audioTrack(
            (libwebrtc::RTCAudioTrack*)(mediaTrack.get()));
        m_track = new AudioStreamTrack(m_executionContext, audioTrack);
    } else if (mediaTrack->kind().std_string() == "video") {
        libwebrtc::scoped_refptr<libwebrtc::RTCVideoTrack> videoTrack(
            (libwebrtc::RTCVideoTrack*)(mediaTrack.get()));
        m_track = new VideoStreamTrack(m_executionContext, videoTrack);
    }

    return m_track;
}

void RTCRtpReceiver::syncStreams()
{
    GCUnorderedMap<libwebrtc::RTCMediaStream*, MediaStream*> curStreams;
    for (auto stream : m_streams) {
        curStreams.insert(std::make_pair(stream->backend().get(), stream));
    }
    m_streams.clear();

    std::vector<libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream>>
        backendStreams = m_backend->streams().std_vector();
    for (auto stream : backendStreams) {
        auto itr = curStreams.find(stream.get());
        if (itr != curStreams.end()) {
            m_streams.push_back(itr->second);
        } else {
            MediaStream* newMediaStream =
                new MediaStream(m_executionContext, stream);
            m_streams.push_back(newMediaStream);
        }
    }
}
} // namespace Starfish

#endif
