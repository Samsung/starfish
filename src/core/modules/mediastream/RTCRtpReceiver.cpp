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

#include "core/modules/mediastream/RTCRtpReceiver.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/MediaStreamTrack.h"
#include "core/modules/mediastream/RTCRtpTransceiver.h"

namespace Starfish {

RTCRtpReceiver::RTCRtpReceiver(
    ExecutionContext* executionContext, RTCRtpTransceiver* transceiver,
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> rtpReceiver)
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

    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> mediaTrack =
        m_backend->track();

    if (!mediaTrack) {
        return nullptr;
    }

    if (mediaTrack->kind() == "audio") {
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack(
            (webrtc::AudioTrackInterface*)(mediaTrack.get()));
        m_track = new AudioStreamTrack(m_executionContext, audioTrack);
    } else if (mediaTrack->kind() == "video") {
        rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(
            (webrtc::VideoTrackInterface*)(mediaTrack.get()));
        m_track = new VideoStreamTrack(m_executionContext, videoTrack);
    }

    return m_track;
}

void RTCRtpReceiver::syncStreams()
{
    GCUnorderedMap<webrtc::MediaStreamInterface*, MediaStream*> curStreams;
    for (auto stream : m_streams) {
        curStreams.insert(std::make_pair(stream->backend().get(), stream));
    }
    m_streams.clear();

    std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>
        backendStreams = m_backend->streams();
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
