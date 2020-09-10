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

#include "core/modules/mediastream/RTCRtpSender.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/MediaStreamTrack.h"
#include "core/modules/mediastream/RTCRtpTransceiver.h"
#include "core/modules/mediastream/RTCPeerConnection.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/page/Navigator.h"
#include "core/page/Window.h"
#include "core/page/WebBase.h"

#include "api/rtp_sender_interface.h"

namespace Starfish {

RTCRtpSender::RTCRtpSender(
    ExecutionContext* executionContext, RTCRtpTransceiver* transceiver,
    rtc::scoped_refptr<webrtc::RtpSenderInterface> rtpSender)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_transceiver(transceiver)
    , m_backend(rtpSender)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCRtpSender*)obj)->~RTCRtpSender(); },
        NULL, NULL, NULL);
}

RTCRtpSender::~RTCRtpSender()
{
    dispose();
}

void RTCRtpSender::dispose()
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
    m_track = nullptr;
    m_transceiver = nullptr;
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

    return m_track;
}

bool RTCRtpSender::setTrack(MediaStreamTrack* track)
{
    m_track = track;

    webrtc::MediaStreamTrackInterface* newTrack = nullptr;

    if (track) {
        if (track->isAudioStreamTrack()) {
            newTrack = track->asAudioStreamTrack()->backend();
        } else if (track->isVideoStreamTrack()) {
            newTrack = track->asVideoStreamTrack()->backend();
        }
    }

    bool r = backend()->SetTrack(newTrack);
    if (!r) {
        STARFISH_LOG_ERROR("%s: failed\n", __func__);
    }
    return r;
}

#if 0 // Disable functions in progress
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
#endif

// https://w3c.github.io/webrtc-pc/#dom-rtcrtpsender-replacetrack
Promise* RTCRtpSender::replaceTrack(MediaStreamTrack* withTrack)
{
    Promise* promise = new Promise(scriptBindingInstance());

    // 1-5
    bool hasValidMediaType = false;
    if (withTrack) {
        if (withTrack->isAudioStreamTrack() &&
            (m_backend->media_type() == cricket::MEDIA_TYPE_AUDIO)) {
            hasValidMediaType = true;
        } else if (withTrack->isVideoStreamTrack() &&
                   (m_backend->media_type() == cricket::MEDIA_TYPE_AUDIO)) {
            hasValidMediaType = true;
        }
    } else {
        hasValidMediaType = true;
    }

    if (!hasValidMediaType) {
        STARFISH_LOG_ERROR("%s: media types do not match\n", __func__);
        auto exception =
            new DOMException(m_executionContext, DOMException::SCRIPT_TYPE_ERR,
                             "media types do not match");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 6.1
    if (m_transceiver->stopped()) {
        STARFISH_LOG_ERROR("%s: transceiver is stopped\n", __func__);
        auto exception = new DOMException(m_executionContext,
                                          DOMException::INVALID_STATE_ERR,
                                          "transceiver is stopped");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 6.2-6.3
    bool sending = false;
    if ((m_transceiver->backend()->current_direction() ==
         webrtc::RtpTransceiverDirection::kSendRecv) ||
        (m_transceiver->backend()->current_direction() ==
         webrtc::RtpTransceiverDirection::kSendOnly)) {
        sending = true;
    }

    // 6.4
    struct Params : public gc {
        RTCRtpSender* self;
        Promise* promise;
        MediaStreamTrack* withTrack;
        bool sending;
    };

    Params* p = new Params();
    p->self = this;
    p->promise = promise;
    p->withTrack = withTrack;
    p->sending = sending;

    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->document()->window(),
        [](size_t handle, void* data) {
            Params* p = (Params*)data;

            if (!p->self->m_transceiver->canSend()) {
                STARFISH_LOG_ERROR("replaceTrack: inactive %d\n",
                                   p->self->m_transceiver->direction());
                auto exception =
                    new DOMException(p->self->m_executionContext,
                                     DOMException::INVALID_MODIFICATION_ERR,
                                     "replaceTrack: inactive");
                p->promise->reject(exception->scriptValue());
            }
            if (p->self->m_transceiver->peerConnection()->isClosed()) {
                return;
            }

            bool r = p->self->setTrack(p->withTrack);
            if (!r) {
                STARFISH_LOG_ERROR("replaceTrack: failed\n");
                auto exception = new DOMException(
                    p->self->m_executionContext,
                    DOMException::INVALID_STATE_ERR, "replaceTrack: failed");
                p->promise->reject(exception->scriptValue());
            }
            p->promise->fulfill(scriptUndefined());
        },
        p);

    return promise;
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
