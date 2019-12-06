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

#include "core/modules/mediastream/RTCRtpSender.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/MediaStreamTrack.h"

#include "api/rtp_sender_interface.h"

namespace Starfish {

RTCRtpSender::RTCRtpSender(ExecutionContext* executionContext)
    : RTCRtpSender(executionContext, nullptr)
{
}

RTCRtpSender::RTCRtpSender(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::RtpSenderInterface> rtpSender)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_backend(rtpSender)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCRtpSender*)obj)->~RTCRtpSender(); },
        NULL, NULL, NULL);
}

RTCRtpSender::~RTCRtpSender()
{
}

ScriptBindingInstance* RTCRtpSender::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

MediaStreamTrack* RTCRtpSender::track()
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

void RTCRtpSender::setTrack(MediaStreamTrack* track)
{
    m_track = track;
    bool r;
    if (track->isAudioStreamTrack()) {
        r = backend()->SetTrack(track->asAudioStreamTrack()->backend());
    } else if (track->isVideoStreamTrack()) {
        r = backend()->SetTrack(track->asVideoStreamTrack()->backend());
    }

    if (!r) {
        STARFISH_LOG_ERROR("%s: failed\n", __func__);
    }
}

RTCDtlsTransport* RTCRtpSender::transport()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

Promise* RTCRtpSender::setParameters(RTCRtpSendParameters parameters)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

RTCRtpSendParameters RTCRtpSender::getParameters()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    RTCRtpSendParameters result;
    return result;
}

Promise* RTCRtpSender::replaceTrack(MediaStreamTrack* withTrack)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

void RTCRtpSender::setStreams(GCVector<MediaStream*>& streams)
{
    std::vector<std::string> streamIds;
    for (auto stream : streams) {
        streamIds.push_back(stream->backend()->id());
    }

    m_backend->SetStreams(streamIds);
}

rtc::scoped_refptr<webrtc::RtpSenderInterface> RTCRtpSender::backend()
{
    return m_backend;
}
}

#endif
