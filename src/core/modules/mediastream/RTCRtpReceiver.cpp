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
}

#endif
