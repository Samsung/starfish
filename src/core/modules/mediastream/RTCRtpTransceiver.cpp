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

#include "core/modules/mediastream/RTCRtpTransceiver.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/modules/mediastream/RTCRtpSender.h"
#include "core/modules/mediastream/RTCRtpReceiver.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/page/Navigator.h"
#include "core/page/Window.h"

namespace Starfish {

RTCRtpTransceiver::RTCRtpTransceiver(
    ExecutionContext* executionContext, RTCPeerConnection* peerConnection,
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> rtpTransceiver)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_peerConnection(peerConnection)
    , m_backend(rtpTransceiver)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((RTCRtpTransceiver*)obj)->~RTCRtpTransceiver();
        },
        NULL, NULL, NULL);
}

RTCRtpTransceiver::~RTCRtpTransceiver()
{
    dispose();
}

void RTCRtpTransceiver::dispose()
{
    if (m_sender) {
        m_sender->dispose();
    }
    if (m_receiver) {
        m_receiver->dispose();
    }
    m_sender = nullptr;
    m_receiver = nullptr;
    m_backend = nullptr;
    m_peerConnection = nullptr;
}

ScriptBindingInstance* RTCRtpTransceiver::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* RTCRtpTransceiver::mid()
{
    if (!m_backend) {
        return nullptr;
    }

    std::string mid = m_backend->mid().std_string();
    if (mid == "") {
        // NOTE: mid() returns "" even if it doesn't have any value internally.
        // currently, we can't determine if it has no value or if it has "".
        return nullptr;
    }

    return String::createASCIIString(mid.c_str(), mid.length());
}

RTCRtpSender* RTCRtpTransceiver::sender()
{
    if (!m_sender) {
        m_sender =
            new RTCRtpSender(m_executionContext, this, m_backend->sender());
    }
    return m_sender;
}

RTCRtpReceiver* RTCRtpTransceiver::receiver()
{
    if (!m_receiver) {
        m_receiver =
            new RTCRtpReceiver(m_executionContext, this, m_backend->receiver());
    }
    return m_receiver;
}

RTCRtpTransceiverDirection RTCRtpTransceiver::direction()
{
    if (stopped()) {
        return RTCRtpTransceiverDirection::Stopped;
    }

    switch (m_backend->direction()) {
    case libwebrtc::RTCRtpTransceiverDirection::kSendRecv:
        return RTCRtpTransceiverDirection::Sendrecv;
    case libwebrtc::RTCRtpTransceiverDirection::kSendOnly:
        return RTCRtpTransceiverDirection::Sendonly;
    case libwebrtc::RTCRtpTransceiverDirection::kRecvOnly:
        return RTCRtpTransceiverDirection::Recvonly;
    default: // case libwebrtc::RTCRtpTransceiverDirection::kInactive:
        return RTCRtpTransceiverDirection::Inactive;
    }
}

void RTCRtpTransceiver::setDirection(RTCRtpTransceiverDirection direction)
{
    if (direction == RTCRtpTransceiverDirection::Sendrecv) {
        m_backend->SetDirectionWithError(
            libwebrtc::RTCRtpTransceiverDirection::kSendRecv);
    } else if (direction == RTCRtpTransceiverDirection::Sendonly) {
        m_backend->SetDirectionWithError(
            libwebrtc::RTCRtpTransceiverDirection::kSendOnly);
    } else if (direction == RTCRtpTransceiverDirection::Recvonly) {
        m_backend->SetDirectionWithError(
            libwebrtc::RTCRtpTransceiverDirection::kRecvOnly);
    } else if (direction == RTCRtpTransceiverDirection::Inactive) {
        m_backend->SetDirectionWithError(
            libwebrtc::RTCRtpTransceiverDirection::kInactive);
    }

    if ((m_backend->current_direction() ==
         libwebrtc::RTCRtpTransceiverDirection::kSendRecv) ||
        (m_backend->current_direction() ==
         libwebrtc::RTCRtpTransceiverDirection::kSendOnly)) {
        m_sentBefore = true;
    }
}

String* RTCRtpTransceiver::directionStr()
{
    if (!m_backend) {
        return String::createASCIIString("stopped");
    }

    switch (m_backend->direction()) {
    case libwebrtc::RTCRtpTransceiverDirection::kSendRecv:
        return String::createASCIIString("sendrecv");
    case libwebrtc::RTCRtpTransceiverDirection::kSendOnly:
        return String::createASCIIString("sendonly");
    case libwebrtc::RTCRtpTransceiverDirection::kRecvOnly:
        return String::createASCIIString("recvonly");
    case libwebrtc::RTCRtpTransceiverDirection::kInactive:
        return String::createASCIIString("inactive");
    default:
        return String::createASCIIString("stopped");
    }
}

void RTCRtpTransceiver::setDirectionStr(String* direction)
{
    if (!m_backend) {
        return;
    }

    if (direction->equals("sendrecv")) {
        setDirection(RTCRtpTransceiverDirection::Sendrecv);
    } else if (direction->equals("sendonly")) {
        setDirection(RTCRtpTransceiverDirection::Sendonly);
    } else if (direction->equals("recvonly")) {
        setDirection(RTCRtpTransceiverDirection::Recvonly);
    } else if (direction->equals("inactive")) {
        setDirection(RTCRtpTransceiverDirection::Inactive);
    }
}

Nullable<String*> RTCRtpTransceiver::currentDirection()
{
    if (!m_backend) {
        return nullptr;
    }

    libwebrtc::RTCRtpTransceiverDirection curDirection =
        m_backend->current_direction();

    switch (curDirection) {
    case libwebrtc::RTCRtpTransceiverDirection::kSendRecv:
        return String::createASCIIString("sendrecv");
    case libwebrtc::RTCRtpTransceiverDirection::kSendOnly:
        return String::createASCIIString("sendonly");
    case libwebrtc::RTCRtpTransceiverDirection::kRecvOnly:
        return String::createASCIIString("recvonly");
    case libwebrtc::RTCRtpTransceiverDirection::kStopped:
        return String::createASCIIString("stopped");
    case libwebrtc::RTCRtpTransceiverDirection::kInactive: {
        // NOTE: current_direction() never return null.
        // This is workaround to return null.
        if (!stopped() && !sentBefore()) {
            return nullptr;
        }

        return String::createASCIIString("inactive");
    }
    }

    return nullptr;
}

bool RTCRtpTransceiver::stopped()
{
    if (!m_backend) {
        return true;
    }

    return m_backend->Stopped();
}

bool RTCRtpTransceiver::sentBefore()
{
    return m_sentBefore;
}

bool RTCRtpTransceiver::canSend()
{
    switch (direction()) {
    case RTCRtpTransceiverDirection::Sendrecv:
    case RTCRtpTransceiverDirection::Sendonly:
        return true;
    default:
        return false;
    }

    return false;
}

void RTCRtpTransceiver::stop()
{
    if (!m_backend) {
        return;
    }

    m_backend->StopInternal();
}

} // namespace Starfish

#endif
