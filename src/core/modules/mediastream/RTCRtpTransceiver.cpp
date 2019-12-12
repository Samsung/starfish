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

#include "core/modules/mediastream/RTCRtpTransceiver.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/mediastream/RTCRtpSender.h"
#include "core/modules/mediastream/RTCRtpReceiver.h"

namespace Starfish {

webrtc::RtpTransceiverInit RTCRtpTransceiverInit::toRtpTransceiverInit()
{
    webrtc::RtpTransceiverInit init;

    if (m_direction->equals("sendrecv")) {
        init.direction = webrtc::RtpTransceiverDirection::kSendRecv;
    } else if (m_direction->equals("sendonly")) {
        init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
    } else if (m_direction->equals("recvonly")) {
        init.direction = webrtc::RtpTransceiverDirection::kRecvOnly;
    } else if (m_direction->equals("inactive")) {
        init.direction = webrtc::RtpTransceiverDirection::kInactive;
    }

    return init;
}

RTCRtpTransceiver::RTCRtpTransceiver(ExecutionContext* executionContext)
    : RTCRtpTransceiver(executionContext, nullptr)
{
}

RTCRtpTransceiver::RTCRtpTransceiver(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtpTransceiver)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_backend(rtpTransceiver)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCRtpTransceiver*)obj)->~RTCRtpTransceiver(); },
        NULL, NULL, NULL);
}

RTCRtpTransceiver::~RTCRtpTransceiver()
{
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

    absl::optional<std::string> mid = m_backend->mid();
    if (!mid.has_value()) {
        return nullptr;
    }

    return String::createASCIIString(mid.value().c_str(), mid.value().length());
}

RTCRtpSender* RTCRtpTransceiver::sender()
{
    if (m_sender) {
        return m_sender;
    }

    m_sender = new RTCRtpSender(m_executionContext, m_backend->sender());
    return m_sender;
}

RTCRtpReceiver* RTCRtpTransceiver::receiver()
{
    if (m_receiver) {
        return m_receiver;
    }

    m_receiver = new RTCRtpReceiver(m_executionContext, m_backend->receiver());
    return m_receiver;
}

RTCRtpTransceiverDirection RTCRtpTransceiver::direction()
{
    if (stopped()) {
        return RTCRtpTransceiverDirection::Stopped;
    }

    switch (m_backend->direction()) {
    case webrtc::RtpTransceiverDirection::kSendRecv:
        return RTCRtpTransceiverDirection::Sendrecv;
    case webrtc::RtpTransceiverDirection::kSendOnly:
        return RTCRtpTransceiverDirection::Sendonly;
    case webrtc::RtpTransceiverDirection::kRecvOnly:
        return RTCRtpTransceiverDirection::Recvonly;
    default: // case webrtc::RtpTransceiverDirection::kInactive:
        return RTCRtpTransceiverDirection::Inactive;
    }
}

void RTCRtpTransceiver::setDirection(RTCRtpTransceiverDirection direction)
{
    if (direction == RTCRtpTransceiverDirection::Sendrecv) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kSendRecv);
    } else if (direction == RTCRtpTransceiverDirection::Sendonly) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kSendOnly);
    } else if (direction == RTCRtpTransceiverDirection::Recvonly) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kRecvOnly);
    } else if (direction == RTCRtpTransceiverDirection::Inactive) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kInactive);
    }

    if ((m_backend->current_direction() ==
         webrtc::RtpTransceiverDirection::kSendRecv) ||
        (m_backend->current_direction() ==
         webrtc::RtpTransceiverDirection::kSendOnly)) {
        m_sentBefore = true;
    }
}

String* RTCRtpTransceiver::directionStr()
{
    if (!m_backend) {
        return String::createASCIIString("stopped");
    }

    switch (m_backend->direction()) {
    case webrtc::RtpTransceiverDirection::kSendRecv:
        return String::createASCIIString("sendrecv");
    case webrtc::RtpTransceiverDirection::kSendOnly:
        return String::createASCIIString("sendonly");
    case webrtc::RtpTransceiverDirection::kRecvOnly:
        return String::createASCIIString("recvonly");
    case webrtc::RtpTransceiverDirection::kInactive:
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

String* RTCRtpTransceiver::currentDirection()
{
    if (!m_backend) {
        return String::emptyString;
    }

    absl::optional<webrtc::RtpTransceiverDirection> curDirection =
        m_backend->current_direction();
    if (!curDirection.has_value()) {
        return String::emptyString;
    }

    switch (curDirection.value()) {
    case webrtc::RtpTransceiverDirection::kSendRecv:
        return String::createASCIIString("sendrecv");
    case webrtc::RtpTransceiverDirection::kSendOnly:
        return String::createASCIIString("sendonly");
    case webrtc::RtpTransceiverDirection::kRecvOnly:
        return String::createASCIIString("recvonly");
    case webrtc::RtpTransceiverDirection::kInactive:
        return String::createASCIIString("inactive");
    }

    return String::emptyString;
}

bool RTCRtpTransceiver::stopped()
{
    if (!m_backend) {
        return true;
    }
    return m_backend->stopped();
}

bool RTCRtpTransceiver::sentBefore()
{
    return m_sentBefore;
}
}

#endif
