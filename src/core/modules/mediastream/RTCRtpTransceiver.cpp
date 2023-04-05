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

libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiverInit>
RTCRtpTransceiverInit::toRtpTransceiverInit()
{
    std::vector<libwebrtc::string> stream_ids;
    libwebrtc::RTCRtpTransceiverDirection dir =
        libwebrtc::RTCRtpTransceiverDirection::kInactive;
    std::vector<libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>>
        encodings;

    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiverInit> init =
        libwebrtc::RTCRtpTransceiverInit::Create(dir, stream_ids, encodings);

    if (m_direction->equals("sendrecv")) {
        init->set_direction(libwebrtc::RTCRtpTransceiverDirection::kSendRecv);
    } else if (m_direction->equals("sendonly")) {
        init->set_direction(libwebrtc::RTCRtpTransceiverDirection::kSendOnly);
    } else if (m_direction->equals("recvonly")) {
        init->set_direction(libwebrtc::RTCRtpTransceiverDirection::kRecvOnly);
    } else if (m_direction->equals("inactive")) {
        init->set_direction(libwebrtc::RTCRtpTransceiverDirection::kInactive);
    }

    return init;
}

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
#if 0 // Disable function in progress
    if (!m_backend) {
        return;
    }

    if (direction->equals("sendrecv")) {
        setDirection(libwebrtc::RTCRtpTransceiverDirection::Sendrecv);
    } else if (direction->equals("sendonly")) {
        setDirection(libwebrtc::RTCRtpTransceiverDirection::Sendonly);
    } else if (direction->equals("recvonly")) {
        setDirection(libwebrtc::RTCRtpTransceiverDirection::Recvonly);
    } else if (direction->equals("inactive")) {
        setDirection(libwebrtc::RTCRtpTransceiverDirection::Inactive);
    }
#endif
}

Nullable<String*> RTCRtpTransceiver::currentDirection()
{
    Nullable<String*> r;
    if (!m_backend) {
        return r;
    }

    libwebrtc::RTCRtpTransceiverDirection curDirection =
        m_backend->current_direction();

    switch (curDirection) {
    case libwebrtc::RTCRtpTransceiverDirection::kSendRecv:
        r = String::createASCIIString("sendrecv");
        break;
    case libwebrtc::RTCRtpTransceiverDirection::kSendOnly:
        r = String::createASCIIString("sendonly");
        break;
    case libwebrtc::RTCRtpTransceiverDirection::kRecvOnly:
        r = String::createASCIIString("recvonly");
        break;
    case libwebrtc::RTCRtpTransceiverDirection::kInactive:
        r = String::createASCIIString("inactive");
        break;
    }
    return r;
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
} // namespace Starfish

#endif
