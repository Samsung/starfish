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

namespace Starfish {

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

String* RTCRtpTransceiver::direction()
{
    if (!m_backend) {
        return String::createASCIIString("stopped");
    }

    switch (m_backend->direction()) {
    case webrtc::RtpTransceiverDirection::kSendRecv:
        return String::createASCIIString("sendrecv");
    case webrtc::RtpTransceiverDirection::kSendOnly:
        return String::createASCIIString("sendOnly");
    case webrtc::RtpTransceiverDirection::kRecvOnly:
        return String::createASCIIString("recvonly");
    case webrtc::RtpTransceiverDirection::kInactive:
        return String::createASCIIString("inactive");
    default:
        return String::createASCIIString("stopped");
    }
}

void RTCRtpTransceiver::setDirection(String* direction)
{
    if (!m_backend) {
        return;
    }

    if (direction->equals("sendrecv")) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kSendRecv);
    } else if (direction->equals("sendonly")) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kSendOnly);
    } else if (direction->equals("recvonly")) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kRecvOnly);
    } else if (direction->equals("inactive")) {
        m_backend->SetDirection(webrtc::RtpTransceiverDirection::kInactive);
    }
}
}

#endif
